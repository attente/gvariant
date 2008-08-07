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
#define GSIZE_FROM_LE GSIZE_TO_LE
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

/*
 * g_variant_serialised_get_child:
 * @container: a #GVariantSerialised
 * @index: the index of the child to fetch
 * @returns: a #GVariantSerialised for the child
 *
 * Extracts a child from a serialised container.
 *
 * It is an error to call this function with an index out of bounds.
 *
 * If the result .data == %NULL and .size > 0 then there has been an
 * error extracting the requested fixed-sized value.  This number of
 * zero bytes needs to be allocated instead.
 *
 * This function will never return .size == 0 and .data != NULL.
 */
GVariantSerialised
g_variant_serialised_get_child (GVariantSerialised container,
                                gsize              index)
{
  g_variant_serialised_assert_invariant (container);

  switch (g_variant_type_info_get_type_class (container.type))
  {
    case G_VARIANT_TYPE_CLASS_VARIANT:
      {
        GVariantSerialised child = { NULL, container.data, container.size };

        if G_UNLIKELY (index > 0)
          break;

        /* find '\0' character */
        while (child.size && container.data[--child.size]);

        /* the loop can finish for two reasons.
         * 1) we found a '\0'.   ((good.))
         * 2) we hit the start.  ((only good if there's a '\0' there))
         */
        if (container.data[child.size] == '\0')
          {
            gchar *str = (gchar *) container.data + child.size + 1;

            /* in the case that we're accessing a shared memory buffer,
             * someone could change the  string under us and cause us
             * to access out-of-bounds memory.
             *
             * if we carefully make our own copy then we avoid that.
             */
            str = g_strndup (str, container.size - child.size - 1);
            if (g_variant_type_string_is_valid (str))
              child.type = g_variant_type_info_get (G_VARIANT_TYPE (str));

            g_free (str);
          }

        if G_UNLIKELY (child.type == NULL)
          {
            child.type = NULL;
            child.data = NULL;
            child.size = 0;

            return child;
          }

        {
          gssize fixed_size;

          g_variant_type_info_query (child.type, NULL, &fixed_size);

          if G_UNLIKELY (fixed_size > 0 && child.size != fixed_size)
            {
              child.size = fixed_size;
              child.data = NULL;
            }
        }

        if (child.size == 0)
          child.data = NULL;

        return child;
      }

    case G_VARIANT_TYPE_CLASS_MAYBE:
      {
        GVariantSerialised child;
        gssize size;

        child.type = g_variant_type_info_element (container.type);
        g_variant_type_info_ref (child.type);
        child.data = container.data;

        if G_UNLIKELY (container.size == 0 || index > 0)
          break;

        g_variant_type_info_query (child.type, NULL, &size);

        if (size > 0)
          {
            /* if this doesn't match then we are considered
             * 'Nothing', so there is no child to get...
             */
            if G_UNLIKELY (container.size != size)
              break;

            /* fixed size: child fills entire container */
            child.size = container.size;
          }
        else
          /* variable size: remove trailing '\0' marker */
          child.size = container.size - 1;

        return child;
      }

    case G_VARIANT_TYPE_CLASS_ARRAY:
      {
        GVariantSerialised child;
        gssize fixed_size;
        guint alignment;

        child.type = g_variant_type_info_element (container.type);
        g_variant_type_info_ref (child.type);

        g_variant_type_info_query (child.type, &alignment, &fixed_size);

        if (fixed_size > 0)
          {
            if G_UNLIKELY (container.size % fixed_size != 0 ||
                           fixed_size * (index + 1) > container.size)
              break;
           
            child.data = container.data + fixed_size * index;
            child.size = fixed_size; 

            return child;
          }

        else
          {
            gsize boundary = 0, start = 0, end = 0;
            guint offset_size;

            offset_size = g_variant_serialiser_offset_size (container);
            memcpy (&boundary,
                    container.data + container.size - offset_size,
                    offset_size);
            boundary = GSIZE_FROM_LE (boundary);
            
            if G_UNLIKELY (boundary > container.size ||
                          (container.size - boundary) % offset_size ||
                          boundary + index * offset_size >= container.size)
              break;

            if (index)
              {
                memcpy (&start,
                        container.data + boundary + (index - 1) * offset_size,
                        offset_size);
                start = GSIZE_FROM_LE (start);
              }

            memcpy (&end,
                    container.data + boundary + index * offset_size,
                    offset_size);
            end = GSIZE_FROM_LE (end);

            start += (-start) & alignment;

            if (start < end && end <= container.size)
              {
                child.data = container.data + start;
                child.size = end - start;
              }
            else
              {
                child.data = NULL;
                child.size = 0;
              }
          }

        return child;
      }

    case G_VARIANT_TYPE_CLASS_STRUCT:
    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      {
        const GVariantMemberInfo *info;
        GVariantSerialised child;
        gsize start = 0, end = 0;
        gsize n_children;
        guint offset_size;
        gssize fixed_size;

        offset_size = g_variant_serialiser_offset_size (container);
        n_children = g_variant_type_info_n_members (container.type);
        info = g_variant_type_info_member_info (container.type, index);
        if (info == NULL)
          break;

        child.type = g_variant_type_info_ref (info->type);
        g_variant_type_info_query (info->type, NULL, &fixed_size);

        if (fixed_size >= 0)
          child.size = fixed_size;
        else
          child.size = 0;
        child.data = NULL;

        if (info->i != -1l)
          {
            if (offset_size * (info->i + 1) > container.size)
              return child;

            memcpy (&start,
                    container.data + container.size - offset_size * (info->i + 1),
                    offset_size);
            start = GSIZE_FROM_LE (start);
          }

        start += info->a;
        start &= info->b;
        start |= info->c;

        if (fixed_size >= 0)
          {
            if (start + fixed_size <= container.size)
              child.data = container.data + start;

            child.data = container.data + start;
            return child;
          }

        if (index == n_children - 1)
          end = container.size - offset_size * (info->i + 1);
        else
          {
            if (offset_size * (info->i + 2) > container.size)
              return child;

            memcpy (&end,
                    container.data + container.size - offset_size * (info->i + 2),
                    offset_size);
            end = GSIZE_FROM_LE (end);
          }

        if (start < end && end <= container.size)
          {
            child.data = container.data + start;
            child.size = end - start;
          }

        return child;
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
        const gchar *type_string;

        g_assert_cmpint (n_children, ==, 1);
        gvs_filler (&child, children[0]);

        type_string = g_variant_type_info_get_string (child.type);

        return child.size + 1 + strlen (type_string);
      }

    case G_VARIANT_TYPE_CLASS_MAYBE:
      {
        GVariantSerialised child = { g_variant_type_info_element (type) };
        gssize fixed_size;

        g_assert_cmpint (n_children, <=, 1);

        if (n_children == 0)
          return 0;

        gvs_filler (&child, children[0]);
        g_variant_type_info_query (child.type, NULL, &fixed_size);

        if (fixed_size >= 0)
          g_assert (child.size == fixed_size);

        if (fixed_size <= 0)
          child.size++;

        return child.size;
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

                if (child.size)
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

gboolean
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
        GVariantSerialised child = { g_variant_type_info_element (value.type),
                                     value.data,
                                     0 };
        gssize fixed_size;
        guint alignment;

        if (value.size == 0)
          return TRUE;

        g_variant_type_info_query (child.type, &alignment, &fixed_size);

        if (fixed_size > 0)
          {
            gsize i;

            if (value.size % fixed_size)
              return FALSE;

            child.size = fixed_size;

            for (i = 0; i < value.size / fixed_size; i++)
              {
                if (!g_variant_serialised_is_normal (child))
                  return FALSE;

                child.data += child.size;
              }

            g_assert (child.data == value.data + value.size);

            return TRUE;
          }
        else
          {
            gsize child_start, child_end;
            gsize length, end, index;
            guint offset_size;

            /* we will need this to check on some things manually */
            offset_size = g_variant_serialiser_offset_size (value);
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
                while (child_start < end && (child_start & alignment))
                  if (value.data[child_start++] != '\0')
                    return FALSE;

                /* ended before alignment, due to cut off buffer */
                if (child_start & alignment)
                  return FALSE;

                /* it ends at the offset given in the offsets */
                if (!g_variant_serialiser_dereference (value,
                                                       length,
                                                       &child_end))
                  return FALSE;

                /* it can't end before it starts... */
                if (child_end < child_start)
                  return FALSE;

                child.data = &value.data[child_start];
                child.size = child_end - child_start;

                if (!g_variant_serialised_is_normal (child))
                  return FALSE;
              }

            g_assert (child_end == end);
          }
      }

    case G_VARIANT_TYPE_CLASS_STRUCT:
    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      {
        return TRUE;
      }

    default:
      g_assert_not_reached ();
  }
}
