/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#include "gvariant-serialiser.h"

#include <glib/gtestutils.h>

#include <string.h>

#ifndef GSIZE_TO_LE
#if GLIB_SIZEOF_SIZE_T == 4
# define GSIZE_TO_LE GUINT32_TO_LE
#else
# define GSIZE_TO_LE GUINT64_TO_LE
#endif
#endif

static gsize
g_variant_serialiser_determine_size (gsize    content_end,
                                     gsize    offsets,
                                     gboolean non_zero)
{
  if (!non_zero && content_end == 0)
    return 0;

  if (content_end + offsets <= G_MAXUINT8)
    return content_end + offsets;

  if (content_end + offsets * 2 <= G_MAXUINT16)
    return content_end + offsets * 2;

  if (content_end + offsets * 4 <= G_MAXUINT32)
    return content_end + offsets * 4;

  return content_end + offsets * 8;
}

static guint
g_variant_serialiser_offset_size (GVariantSerialised container)
{
  if (container.size == 0)
    return 0;

  if (container.size <= G_MAXUINT8)
    return 1;

  if (container.size <= G_MAXUINT16)
    return 2;

  if (container.size <= G_MAXUINT32)
    return 4;

  return 8;
}

#define do_case(_ofs_size, body) \
  {                                                             \
    guint offset_size = (_ofs_size);                            \
    body                                                        \
  }

#define check_case(max, _ofs_size, body) \
  if (container.size <= max)                                    \
    do_case (_ofs_size, body)

#define check_cases(init_val, zero_case, else_case, use_val) \
  G_STMT_START {                                                \
    guchar *bytes;                                              \
    union                                                       \
    {                                                           \
      guchar bytes[GLIB_SIZEOF_SIZE_T];                         \
      gsize integer;                                            \
    } tmpvalue;                                                 \
                                                                \
    bytes = container.data + container.size;                    \
    tmpvalue.integer = init_val;                                \
                                                                \
    if (container.size == 0) { zero_case }                      \
    else check_case (G_MAXUINT8, 1, else_case)                  \
    else check_case (G_MAXUINT16, 2, else_case)                 \
    else check_case (G_MAXUINT32, 4, else_case)                 \
    else do_case (8, else_case)                                 \
    use_val;                                                    \
  } G_STMT_END

static gboolean
g_variant_serialiser_dereference (GVariantSerialised container,
                                  gsize              index,
                                  gsize             *result)
{
  check_cases (0,,
               {
                 if (index >= container.size / offset_size)
                   return FALSE;

                 bytes -= (index + 1) * offset_size;
                 memcpy (tmpvalue.bytes, bytes, offset_size);
               },
               *result = GSIZE_TO_LE (tmpvalue.integer));

  return G_LIKELY (*result <= container.size);
}

static gboolean
g_variant_serialiser_array_length (GVariantSerialised  container,
                                   gsize              *length_ret)
{
  gsize divider;

  check_cases (0, g_assert_not_reached ();,
               {
                 bytes -= offset_size;
                 memcpy (tmpvalue.bytes, bytes, offset_size);
                 divider = offset_size;
               },
               {
                 gsize length;

                 length = GSIZE_TO_LE (tmpvalue.integer);

                 if (length > container.size)
                   return FALSE;

                 length = container.size - length;
                 if G_UNLIKELY (length % divider != 0)
                   return FALSE;

                 *length_ret = length / divider;
               });

  return TRUE;
}
/*
static void
g_variant_serialiser_sanity_check (GVariantSerialised container,
                                   gsize              offset,
                                   gsize              n_items)
{
  check_cases (0,
               {
                 if (offset)
                   {
                     g_error ("when serialising a zero-size container, %d "
                              "bytes were written", offset);
                   }
               },
               {
                 if (offset + n_items * offset_size != container.size)
                   {
                     g_error ("when serialising a container of size %d "
                              "(offset size %d) %d bytes were used by "
                              "children and %d bytes by %d offsets (total "
                              "of %d bytes).", container.size, offset_size,
                              offset, n_items * offset_size, n_items,
                              offset + n_items * offset_size);
                   }
               },);
}*/

static gsize
g_variant_serialiser_struct_end (GVariantSerialised container,
                                 gsize              n_offsets)
{
  check_cases (0, return container.size;,
                  return container.size - (n_offsets * offset_size); ,);
}

static GVariantSerialised
g_variant_serialiser_error (GVariantSerialised  container,
                            GVariantTypeInfo   *type)
{
  GVariantSerialised result = { type, NULL, 0 };

  g_assert_not_reached ();

  return result;
}

static GVariantSerialised
g_variant_serialiser_sub (GVariantSerialised  container,
                          GVariantTypeInfo   *type,
                          gsize               start,
                          gsize               end)
{
  GVariantSerialised result = { g_variant_type_info_ref (type) };

  if G_LIKELY (start <= end && end <= container.size)
    {
      result.data = &container.data[start];
      result.size = end - start;
    }
  else
    {
      result.data = NULL;
      result.size = 0;
    }

  return result;
}

gsize
g_variant_serialised_n_children (GVariantSerialised container)
{
  g_variant_serialised_assert_invariant (container);

  switch (g_variant_type_info_get_type_class (container.type))
  {
    case G_VARIANT_TYPE_CLASS_VARIANT:
      return 1;

    case G_VARIANT_TYPE_CLASS_STRUCT:
      return g_variant_type_info_n_members (container.type);

    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      return 2;

    case G_VARIANT_TYPE_CLASS_MAYBE:
      {
        gssize size;

        if (container.size == 0)
          return 0;

        g_variant_type_info_query_element (container.type, NULL, &size);

        if (size > 0 && size != container.size)
          break;

        return 1;
      }

    case G_VARIANT_TYPE_CLASS_ARRAY:
      {
        gssize size;

        /* an array with a length of zero always has a size of zero.
         * an array with a size of zero always has a length of zero.
         */
        if (container.size == 0)
          return 0;

        g_variant_type_info_query_element (container.type, NULL, &size);

        if (size <= 0)
          /* case where array contains variable-sized elements
           * or fixed-size elements of size 0 (treated as variable)
           */
          {
            gsize length;

            if G_UNLIKELY (!g_variant_serialiser_array_length (container, &length))
              break;

            return length;
         }
        else
          /* case where array contains fixed-sized elements */
          {
            if G_UNLIKELY (container.size % size > 0)
              break;

            return container.size / size;
          }
      }

    default:
      g_assert_not_reached ();
  }

  g_error ("deserialise error on n_children");
  return 0;
}

GVariantSerialised
g_variant_serialised_get_child (GVariantSerialised container,
                                gsize              index)
{
  g_variant_serialised_assert_invariant (container);

  switch (g_variant_type_info_get_type_class (container.type))
  {
    case G_VARIANT_TYPE_CLASS_MAYBE:
      {
        GVariantTypeInfo *type;
        gssize size;

        type = g_variant_type_info_element (container.type);

        if G_UNLIKELY (container.size == 0 || index > 0)
          break;

        g_variant_type_info_query (type, NULL, &size);

        if (size > 0)
          {
            if G_UNLIKELY (container.size != size)
              break;

            /* fixed size: do nothing */
          }
        else
          /* variable size: remove trailing '\0' marker */
          container.size -= 1;

        return g_variant_serialiser_sub (container, type, 0, container.size);
      }

    case G_VARIANT_TYPE_CLASS_ARRAY:
      {
        GVariantTypeInfo *type;
        guint alignment;
        gssize size;

        type = g_variant_type_info_element (container.type);
        g_variant_type_info_query (type, &alignment, &size);

        if (size <= 0)
          /* case where array contains variable-sized elements */
          {
            gsize start = 0, end;
            gsize length;

            if G_UNLIKELY (!g_variant_serialiser_array_length (container,
                                                               &length))
              break;

            if G_UNLIKELY (index >= length)
              break;

            if (index &&
               !g_variant_serialiser_dereference (container,
                                                  length - index, &start))
              return g_variant_serialiser_error (container, type);

            if (!g_variant_serialiser_dereference (container,
                                                   length - index - 1, &end))
              return g_variant_serialiser_error (container, type);

            start += (-start) & alignment;

            return g_variant_serialiser_sub (container, type, start, end);
          }
        else
          /* case where array contains fixed-sized elements */
          {
            if G_UNLIKELY (container.size % size > 0 ||
                            size * (index + 1) > container.size)
              break;

            return g_variant_serialiser_sub (container, type,
                                             size * index, size * (index + 1));
          }
      }

    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
    case G_VARIANT_TYPE_CLASS_STRUCT:
      {
        const GVariantMemberInfo *info;
        gsize start = 0, end = -1;
        gssize fixed_size;

        if (!(info = g_variant_type_info_member_info (container.type, index)))
          break;

        if (info->i != -1)
          if (!g_variant_serialiser_dereference (container, info->i, &start))
            return g_variant_serialiser_error (container, info->type);

        start += info->a;
        start &= info->b;
        start |= info->c;

        g_variant_type_info_query (info->type, NULL, &fixed_size);

        if (fixed_size >= 0)
          end = start + fixed_size;
        else if (index == g_variant_type_info_n_members (container.type) - 1)
          end = g_variant_serialiser_struct_end (container, 1 + info->i);
        else
          {
            if (!g_variant_serialiser_dereference (container,
                                                   info->i + 1, &end))
              return g_variant_serialiser_error (container, info->type);
          }

        return g_variant_serialiser_sub (container, info->type, start, end);
      }

    case G_VARIANT_TYPE_CLASS_VARIANT:
      {
        GVariantSerialised result;
        GVariantTypeInfo *info;
        gchar *type_string;
        gssize expected;
        gsize end;

        if G_UNLIKELY (index != 0)
          break;

        /* we need to string copy here just incase someone changes the
         * data under us while we're reading (think: shared memory)
         */
        end = container.size;
        while (end && container.data[--end]);

        if (container.data[end] == '\0')
          type_string = g_strndup ((const gchar *) &container.data[end + 1],
                                   container.size - (end + 1));
        else
          type_string = NULL;

        g_print ("i see '%s'\n", type_string);

        if (type_string && g_variant_type_string_is_valid (type_string))
          info = g_variant_type_info_get (G_VARIANT_TYPE (type_string));
        else
          info = g_variant_type_info_get (G_VARIANT_TYPE_UNIT);

        g_free (type_string);

        g_variant_type_info_query (info, NULL, &expected);

        if (expected >= 0 && end != expected)
          result = g_variant_serialiser_error (container, info);
        else
          result = g_variant_serialiser_sub (container, info, 0, end);

        g_variant_type_info_unref (info);

        return result;
      }

    default:
      g_assert_not_reached ();
  }

  g_error ("Attempt to access item %d in a container with only %d items",
           index, g_variant_serialised_n_children (container));
}

void
g_variant_serialiser_serialise (GVariantSerialised        container,
                                GVariantSerialisedFiller  gvs_filler,
                                const gpointer           *children,
                                gsize                     n_children)
{
  switch (g_variant_type_info_get_type_class (container.type))
  {
    case G_VARIANT_TYPE_CLASS_VARIANT:
      {
        GVariantSerialised child = { NULL, container.data, 0 };
        const gchar *type_string;
        gsize type_length;

        g_assert_cmpint (n_children, ==, 1);

        /* write the data for the child.
         * side effect: child.type and child.size are set
         */
        gvs_filler (&child, children[0]);

        type_string = g_variant_type_info_get_string (child.type);
        type_length = strlen (type_string);

        /* write the separator byte */
        container.data[child.size] = '\0';

        /* and the type string */
        memcpy (container.data + child.size + 1, type_string, type_length);

        /* make sure that it all adds up */
        g_assert_cmpint (child.size + 1 + type_length, ==, container.size);

        return;
      }

    case G_VARIANT_TYPE_CLASS_MAYBE:
      {
        g_assert_cmpint (n_children, ==, (container.size > 0));

        if (n_children)
          {
            GVariantSerialised child = { NULL, container.data, 0 };
            gssize fixed_size;

            child.type = g_variant_type_info_element (container.type);
            g_variant_type_info_query (child.type, NULL, &fixed_size);

            /* write the data for the child.
             * side effect: asserts child.type matches
             * also: sets child.size
             */
            gvs_filler (&child, children[0]);

            /* for all fixed sized children, double check */
            if (fixed_size >= 0)
              g_assert_cmpint (fixed_size, ==, child.size);

            /* for variable-width or 0-size children, add a pad byte */
            if (fixed_size <= 0)
              container.data[child.size++] = '\0';

            g_assert_cmpint (child.size, ==, container.size);
          }

        return;
      }

    case G_VARIANT_TYPE_CLASS_ARRAY:
      {
        g_assert_cmpint ((n_children > 0), ==, (container.size > 0));

        if (n_children)
          {
            GVariantSerialised child = { NULL, container.data, 0 };
            guchar *offset_bound, *offset_ptr;
            guint offset_size, alignment;
            gssize fixed_size;

            child.type = g_variant_type_info_element (container.type);
            g_variant_type_info_query (child.type, &alignment, &fixed_size);
            offset_size = g_variant_serialiser_offset_size (container);
            offset_bound = container.data + container.size;
            child.data = container.data;

            if (fixed_size <= 0)
              offset_bound -= offset_size * n_children;
            offset_ptr = offset_bound;

            while (n_children--)
              {
                g_assert (container.data <= child.data       &&
                          child.data <= offset_bound         &&
                          offset_bound <= offset_ptr         &&
                          offset_ptr <= container.data + container.size);

                if (child.data < offset_bound)
                  while (((gsize) child.data) & alignment)
                    *child.data++ = '\0';

                gvs_filler (&child, *children++);
                child.data += child.size;
                child.size = 0;

                if (fixed_size <= 0)
                  {
                    gsize offset = GSIZE_TO_LE (child.data - container.data);
                    memcpy (offset_ptr, &offset, offset_size);
                    offset_ptr += offset_size;
                  }
              }

            g_assert (child.data == offset_bound &&
                      offset_ptr == container.data + container.size);
          }

        return;
      }

    case G_VARIANT_TYPE_CLASS_STRUCT:
    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      {
        g_assert_cmpint ((n_children > 0), >=, (container.size > 0));

        if (n_children)
          {
            gsize offset, offsets_bound, offsets_ptr;
            const GVariantMemberInfo *info;
            guint offset_size, align;
            gssize fixed_size;
            gboolean fixed;

            info = g_variant_type_info_member_info (container.type, 0);
            offset_size = g_variant_serialiser_offset_size (container);

            offset = 0;
            fixed = TRUE;
            offsets_bound = offsets_ptr = container.size;
            offsets_bound -= (info[n_children - 1].i + 1) * offset_size;

            while (n_children--)
              {
                GVariantSerialised child = { info++->type };

                g_variant_type_info_query (child.type, &align, &fixed_size);
                child.data = container.data + offset + ((-offset) & align);
                gvs_filler (&child, *children++);

                while (child.size &&& container.data[offset] != child.data)
                  container.data[offset++] = '\0';
                offset += child.size;

                if (fixed_size < 0)
                  {
                    fixed = FALSE;

                    if (n_children)
                      {
                        gsize le_offset = GSIZE_TO_LE (offset);
                        
                        offsets_ptr -= offset_size;
                        memcpy (&container.data[offsets_ptr],
                                &le_offset, offset_size);
                      }
                  }
              }

            g_variant_type_info_query (container.type, &align, &fixed_size);

            if (fixed)
              {
                while (offset & align)
                  container.data[offset++] = '\0';

                g_assert_cmpint (offsets_bound, ==, container.size);
                g_assert_cmpint (offset, ==, fixed_size);
              }
            else
              g_assert (fixed_size == -1);

            g_assert_cmpint (offset, ==, offsets_bound);
            g_assert_cmpint (offsets_ptr, ==, offsets_bound);
          }

        return;
      }

    default:
      g_assert_not_reached ();
  }
}

gsize
g_variant_serialiser_needed_size (GVariantTypeInfo         *type,
                                  GVariantSerialisedFiller  gvs_filler,
                                  const gpointer           *children,
                                  gsize                     n_children)
{
  switch (g_variant_type_info_get_type_class (type))
  {
    case G_VARIANT_TYPE_CLASS_VARIANT:
      {
        GVariantSerialised child = {};

        g_assert_cmpint (n_children, ==, 1);
        gvs_filler (&child, children[0]);

        return child.size + 1 +
               g_variant_type_get_string_length (
                 g_variant_type_info_get_type (child.type));
      }

    case G_VARIANT_TYPE_CLASS_ARRAY:
      {
        GVariantTypeInfo *elem_type;
        gssize fixed_size;
        guint alignment;

        if (n_children == 0)
          return 0;

        elem_type = g_variant_type_info_element (type);
        g_variant_type_info_query (elem_type, &alignment, &fixed_size);

        if (fixed_size <= 0)
          {
            gsize offset;
            gsize i;

            offset = 0;

            for (i = 0; i < n_children; i++)
              {
                GVariantSerialised child = {};

                gvs_filler (&child, children[i]);
                g_assert (child.type == elem_type);

                offset += (-offset) & alignment;
                offset += child.size;
              }

            return g_variant_serialiser_determine_size (offset,
                                                        n_children,
                                                        TRUE);
          }
        else
          return fixed_size * n_children;
      }

    case G_VARIANT_TYPE_CLASS_MAYBE:
      {
        GVariantSerialised child = {};
        GVariantTypeInfo *elem_type;
        gssize fixed_size;

        g_assert_cmpint (n_children, <=, 1);

        if (n_children == 0)
          return 0;

        elem_type = g_variant_type_info_element (type);
        g_variant_type_info_query (elem_type, NULL, &fixed_size);
        gvs_filler (&child, children[0]);
        g_assert (child.type == elem_type);

        if (fixed_size > 0)
          return fixed_size;
        else
          return child.size + 1;
      }

    case G_VARIANT_TYPE_CLASS_STRUCT:
    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      {
        gsize n_offsets;
        gsize offset;
        gsize i;

        g_assert_cmpint (g_variant_type_info_n_members (type), ==, n_children);

        {
          gssize fixed_size;

          g_variant_type_info_query (type, NULL, &fixed_size);

          if (fixed_size >= 0)
            return fixed_size;
        }

        n_offsets = 0;
        offset = 0;

        for (i = 0; i < n_children; i++)
          {
            GVariantSerialised child = {};
            const GVariantMemberInfo *info;
            guint alignment;
            gssize size;

            info = g_variant_type_info_member_info (type, i);
            gvs_filler (&child, children[i]);
            g_assert (child.type == info->type);

            g_variant_type_info_query (info->type, &alignment, &size);

            if (size < 0)
              {
                if (child.size)
                  {
                    offset += (-offset) & alignment;
                    offset += child.size;
                  }

                if (i != n_children - 1)
                  n_offsets++;
              }
            else
              {
                offset += (-offset) & alignment;
                offset += size;
              }
          }

          /* no need to pad fixed-sized structures here because
           * fixed sized structures are taken care of directly.
           */

        return g_variant_serialiser_determine_size (offset, n_offsets, FALSE);
      }

    default:
      g_assert_not_reached ();
  }
}

void
g_variant_serialised_byteswap (GVariantSerialised value)
{
  gssize fixed_size;
  guint alignment;

  if (!value.data)
    return;

  /* the types we potentially need to byteswap are
   * exactly those with alignment requirements.
   */
  g_variant_type_info_query (value.type, &alignment, &fixed_size);
  if (!alignment)
    return;

  /* if fixed size and alignment are equal then we are down
   * to the base integer type and we should swap it.  the
   * only exception to this is if we have a struct with a
   * single item, and then swapping it will be OK anyway.
   */
  if (alignment + 1 == fixed_size)
    {
      switch (fixed_size)
      {
        case 2:
          {
            guint16 *ptr = (guint16 *) value.data;

            g_assert_cmpint (value.size, ==, 2);
            *ptr = GUINT16_SWAP_LE_BE (*ptr);
          }
          return;

        case 4:
          {
            guint32 *ptr = (guint32 *) value.data;

            g_assert_cmpint (value.size, ==, 4);
            *ptr = GUINT32_SWAP_LE_BE (*ptr);
          }
          return;

        case 8:
          {
            guint64 *ptr = (guint64 *) value.data;

            g_assert_cmpint (value.size, ==, 8);
            *ptr = GUINT64_SWAP_LE_BE (*ptr);
          }
          return;

        default:
          g_assert_not_reached ();
      }
    }

  /* else, we have a container that potentially contains
   * some children that need to be byteswapped.
   */
  else
    {
      gsize children, i;
      
      children = g_variant_serialised_n_children (value);
      for (i = 0; i < children; i++)
        {
          GVariantSerialised child;

          child = g_variant_serialised_get_child (value, i);
          g_variant_serialised_byteswap (child);
        }
    }
}

void
g_variant_serialised_assert_invariant (GVariantSerialised value)
{
  gssize fixed_size;
  guint alignment;

  g_assert (value.type != NULL);
  g_assert_cmpint ((value.data == NULL), <=, (value.size == 0));

  g_variant_type_info_query (value.type, &alignment, &fixed_size);

  g_assert_cmpint (((gsize) value.data) & alignment, ==, 0);
  if (fixed_size >= 0)
    g_assert_cmpint (value.size, ==, fixed_size);
}

#if 0
static gboolean
g_variant_serialised_is_normal (GVariantSerialised value)
{
  switch (g_variant_type_info_get_type_class (value.type))
  {
    case G_VARIANT_TYPE_CLASS_BYTE:
    case G_VARIANT_TYPE_CLASS_INT16:
    case G_VARIANT_TYPE_CLASS_UINT16:
    case G_VARIANT_TYPE_CLASS_INT32:
    case G_VARIANT_TYPE_CLASS_UINT32:
    case G_VARIANT_TYPE_CLASS_INT64:
    case G_VARIANT_TYPE_CLASS_UINT64:
    case G_VARIANT_TYPE_CLASS_DOUBLE:
      return TRUE;

    case G_VARIANT_TYPE_CLASS_BOOLEAN:
      return value.data[0] == FALSE || value.data[0] == TRUE;

    case G_VARIANT_TYPE_CLASS_STRING:
      return value.size > 0 && value.data[value.size - 1] == '\0' &&
             strlen ((const char *) value.data) + 1 == value.size;

    case G_VARIANT_TYPE_CLASS_MAYBE:
      {
        GVariantTypeInfo *element;
        gssize fixed_size;

        /* Nothing case */
        if (value.size == 0)
          return TRUE;

        /* Just case */
        element = g_variant_type_info_element (value.type);
        g_variant_type_info_query (element, NULL, &fixed_size);

        if (fixed_size > 0)
          {
            /* if element is fixed size, Just must be the same */
            return value.size == fixed_size;
          }
        else
          {
            /* if element is variable size, we have a zero pad */
            if (value.data[value.size - 1] != '\0')
              return FALSE;

            /* and the element itself should check out */
            value.type = element;
            value.size--;

            return g_variant_serialised_is_normal (value);
          }
      }

    case G_VARIANT_TYPE_CLASS_ARRAY:
      {
        GVariantTypeInfo *element;
        gssize fixed_size;
        guint alignment;

        if (value.size == 0)
          return TRUE;

        element = g_variant_type_info_element (value.type);
        g_variant_type_info_query (element, &alignment, &fixed_size);

        if (fixed_size > 0)
          {
            gsize i;

            if (value.size % fixed_size)
              return FALSE;

            for (i = 0; i < value.size / fixed_size; i++)
              {
                GVariantSerialised child = { element,
                                             &value.data[fixed_size * i],
                                             fixed_size };
                
                if (!g_variant_serialised_is_normal (child))
                  return FALSE;
              }

            return TRUE;
          }
        else
          {
            gsize child_start, child_end;
            gsize length, end, index;
            guint offset_size;
            gboolean success;

            /* we will need this to check on some things manually */
            offset_size = g_variant_serialiser_get_offset_size (value);
            g_assert (offset_size > 0);

            /* make sure the end offset is in-bounds */
            if (!g_variant_serialiser_dereference (value, 0, &end))
              return FALSE;

            /* make sure we have an integer number of offsets */
            if ((value.size - end) % offset_size)
              return FALSE;

            /* number of offsets = length of the array */
            length = (value.size - end) / offset_size;

            /* ensure that the smallest possible offset size was chosen */
            if (value.size !=
                g_variant_serialiser_determine_size (end, length, TRUE))
              return FALSE;

            child_end = 0;
            index = 0;
            while (length--)
              {
                GVariantSerialised child;

                /* this element starts past the end of the last one... */
                child_start = child_end;

                /* ...after adding padding bytes for the alignment */
                while (child_start < value.size && (child & alignment))
                  if (value.data[child_start++] != '\0')
                    return FALSE;

                /* ended before alignment, due to cut off buffer */
                if (child & alignment)

                /* it ends at the offset given in the offsets */
                if (!g_variant_serialiser_dereference (value,
                                                       length,
                                                       &child_end))
                  return FALSE;

                if (child_end < child_start)


                child.type = element;
                child.data = &value.data[child_start];




              }

            for (i = 0; i < length; i++)
              {
              }
            
            g_assert_not_reached (); /* XXX */
          }
      }

    case G_VARIANT_TYPE_CLASS_STRUCT:
    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      {

      }
  }

}
#endif

gboolean
g_variant_serialised_is_normalised (GVariantSerialised value)
{
  switch (g_variant_type_info_get_type_class (value.type))
  {
    case G_VARIANT_TYPE_CLASS_BYTE:
    case G_VARIANT_TYPE_CLASS_INT16:
    case G_VARIANT_TYPE_CLASS_UINT16:
    case G_VARIANT_TYPE_CLASS_INT32:
    case G_VARIANT_TYPE_CLASS_UINT32:
    case G_VARIANT_TYPE_CLASS_INT64:
    case G_VARIANT_TYPE_CLASS_UINT64:
    case G_VARIANT_TYPE_CLASS_DOUBLE:
      return value.data != NULL;

    case G_VARIANT_TYPE_CLASS_BOOLEAN:
      return value.data && (value.data[0] == FALSE ||
                            value.data[0] == TRUE);

    case G_VARIANT_TYPE_CLASS_SIGNATURE:
    case G_VARIANT_TYPE_CLASS_OBJECT_PATH:
      g_error ("don't validate these yet");

    case G_VARIANT_TYPE_CLASS_STRING:
      if (value.size == 0)
        return FALSE;

      if (value.data[value.size - 1])
        return FALSE;

      return strlen ((const char *) value.data) + 1 != value.size;

    case G_VARIANT_TYPE_CLASS_ARRAY:
    case G_VARIANT_TYPE_CLASS_MAYBE:
    case G_VARIANT_TYPE_CLASS_STRUCT:
    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
    case G_VARIANT_TYPE_CLASS_VARIANT:
      {
        GVariantSerialised last;
        gsize children, i;

        children = g_variant_serialised_n_children (value);
        for (i = 0; i < children; i++)
          {
            GVariantSerialised child;
            guchar *zero;

            child = g_variant_serialised_get_child (value, i);

            if (child.data == NULL)
              return FALSE;

            if (i && child.data < &last.data[last.size])
              return FALSE;

            for (zero = &last.data[last.size]; zero < child.data; zero++)
              if (*zero != '\0')
                return FALSE;

            if (!g_variant_serialised_is_normalised (child))
              return FALSE;

            last = child;
          }
        break;
      }

    default:
      g_assert_not_reached ();
  }

  return TRUE;
}
