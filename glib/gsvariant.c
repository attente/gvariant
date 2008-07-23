/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#include "gsvariant.h"

#include <string.h>
#include <glib.h>

#if GLIB_SIZEOF_SIZE_T == 4
# define GSIZE_TO_LE GUINT32_TO_LE
#else
# define GSIZE_TO_LE GUINT64_TO_LE
#endif

static gsize
g_svariant_determine_size (gsize    content_end,
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
g_svariant_dereference (GSVariant  container,
                        gsize      index,
                        gsize     *result)
{
  check_cases (0,,
               {
                 if (index >= container.size / offset_size)
                   return FALSE;

                 bytes -= (index + 1) * offset_size;
                 memcpy (tmpvalue.bytes, bytes, offset_size);
               },
               *result = GSIZE_TO_LE (tmpvalue.integer));

  return TRUE;
}

static void
g_svariant_assign (GSVariant  container,
                   gsize      index,
                   gsize      value)
{
  check_cases (GSIZE_TO_LE (value),,
               {
                 g_assert (index < container.size / offset_size);

                 bytes -= (index + 1) * offset_size;
                 memcpy (bytes, tmpvalue.bytes, offset_size);
               },);
}

static gboolean
g_svariant_array_length (GSVariant  container,
                         gsize     *length_ret)
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
                 if (G_UNLIKELY (length % divider != 0))
                   return FALSE;

                 *length_ret = length;
               });

  return TRUE;
}

static void
g_variant_serialiser_sanity_check (GSVariant container,
                                   gsize     offset,
                                   gsize     n_items)
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
}

static gsize
g_svariant_struct_end (GSVariant container,
                       gsize     n_offsets)
{
  check_cases (0, return container.size;,
                  return container.size - (n_offsets * offset_size); ,);
}

static void
g_svariant_pad (GSVariant  container,
                gsize     *offset,
                gint       alignment)
{
  while (*offset & alignment)
    container.data[(*offset)++] = '\0';
}

static GSVariant
g_svariant_error (GSVariant  container,
                  GSVHelper *helper)
{
  g_assert_not_reached ();
  return g_svariant (helper, NULL, 0);
}

static GSVariant
g_svariant_sub (GSVariant  container,
                GSVHelper *helper,
                gsize      start,
                gsize      end)
{
  if (G_UNLIKELY (end < start || container.size < end))
    return g_svariant_error (container, helper);

  g_svhelper_ref (helper);

  if (end == start)
    return g_svariant (helper, NULL, 0);
  else
    return g_svariant (helper, container.data + start, end - start);
}

static GSVariant
g_svariant_sub_checked (GSVariant         container,
                        GSVHelper *helper,
                        gsize             start,
                        gsize             end)
{
  gsize size = end - start;
  gssize expected;

  g_svhelper_info (helper, NULL, &expected);

  if (G_UNLIKELY (expected >= 0 && expected != size))
    return g_svariant_error (container, helper);

  return g_svariant_sub (container, helper, start, end);
}

gsize
g_svariant_n_children (GSVariant container)
{
  switch (g_svhelper_type (container.helper))
  {
    case G_SIGNATURE_TYPE_VARIANT:
      return 1;

    case G_SIGNATURE_TYPE_STRUCT:
      return g_svhelper_n_members (container.helper);

    case G_SIGNATURE_TYPE_DICT_ENTRY:
      return 2;

    case G_SIGNATURE_TYPE_MAYBE:
      {
        gssize size;

        if (container.size == 0)
          return 0;

        g_svhelper_element_info (container.helper, NULL, &size);

        if (size > 0 && size != container.size)
          break;

        return container.size > 0;
      }

    case G_SIGNATURE_TYPE_ARRAY:
      {
        gssize size;

        /* an array with a length of zero always has a size of zero.
         * an array with a size of zero always has a length of zero.
         */
        if (container.size == 0)
          return 0;

        g_svhelper_element_info (container.helper, NULL, &size);

        if (size <= 0)
          /* case where array contains variable-sized elements
           * or fixed-size elements of size 0 (treated as variable)
           */
          {
            gsize length;

            if G_UNLIKELY (!g_svariant_array_length (container, &length))
              break;

            return length;
         }
        else
          /* case where array contains fixed-sized elements */
          {
            if (G_UNLIKELY (container.size % size > 0))
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

GSVariant
g_svariant_get_child (GSVariant container,
                      gsize     index)
{
  switch (g_svhelper_type (container.helper))
  {
    case G_SIGNATURE_TYPE_MAYBE:
      {
        GSVHelper *signature;
        gssize size;

        signature = g_svhelper_element (container.helper);

        if (G_UNLIKELY (container.size == 0 || index > 0))
          break;

        g_svhelper_info (signature, NULL, &size);

        if (size > 0)
          {
            if (G_UNLIKELY (container.size != size))
              break;

            /* fixed size: do nothing */
          }
        else
          /* variable size: remove trailing '\0' marker */
          container.size -= 1;

        return g_svariant_sub (container, signature, 0, container.size);
      }

    case G_SIGNATURE_TYPE_ARRAY:
      {
        GSVHelper *signature;
        guint alignment;
        gssize size;

        signature = g_svhelper_element (container.helper);
        g_svhelper_info (signature, &alignment, &size);

        if (size <= 0)
          /* case where array contains variable-sized elements */
          {
            gsize start = 0, end;
            gsize length;

            if (G_UNLIKELY (!g_svariant_array_length (container, &length)))
              break;

            if (G_UNLIKELY (index >= length))
              break;

            if (index &&
               !g_svariant_dereference (container, length - index, &start))
              return g_svariant_error (container, signature);

            if (!g_svariant_dereference (container, length - index - 1, &end))
              return g_svariant_error (container, signature);

            start += (-start) & alignment;

            return g_svariant_sub (container, signature, start, end);
          }
        else
          /* case where array contains fixed-sized elements */
          {
            if (G_UNLIKELY (container.size % size > 0 ||
                            size * (index + 1) > container.size))
              break;

            //g_print ("%p fixed (%d) -> %d and %d\n", signature, index, size * index, size * (index + 1));
            return g_svariant_sub (container, signature,
                                   size * index, size * (index + 1));
          }
      }

    case G_SIGNATURE_TYPE_DICT_ENTRY:
    case G_SIGNATURE_TYPE_STRUCT:
      {
        gsize start = 0, end = -1;
        MemberInfo info;

        if (!g_svhelper_member_info (container.helper,
                                             index, &info))
          break;

        if (info.index != -1)
          if (!g_svariant_dereference (container, info.index, &start))
            return g_svariant_error (container, info.helper);

        //g_print ("%d -> *%d +%x &%x |%x\n", index, info.index, info.plus, info.and, info.or);
        start += info.plus;
        start &= info.and;
        start |= info.or;

        if (info.size == STRUCT_MEMBER_LAST)
          {
            end = g_svariant_struct_end (container, 1 + info.index);
          }
        else if (info.size == STRUCT_MEMBER_VARIABLE)
          {
            if (!g_svariant_dereference (container, info.index + 1, &end))
              return g_svariant_error (container, info.helper);
          }
        else
          end = start + info.size;

//        g_print ("from %d to %d\n", start, end);
        return g_svariant_sub (container, info.helper, start, end);
      }

    case G_SIGNATURE_TYPE_VARIANT:
      {
        char *signature = NULL;
        GSVHelper *helper;
        GSVariant result;
        gsize end;

        if (G_UNLIKELY (index != 0))
          break;

        /* TODO: can probably do this faster (ie: no copy...) */
        end = container.size;
        while (end && container.data[--end]);

        if (end)
          signature = g_strndup ((const char *) &container.data[end + 1],
                                 container.size - (end + 1));

        if (signature && g_signature_is_valid (signature))
          helper = g_svhelper_get (g_signature (signature));
        else
          helper = g_svhelper_get (G_SIGNATURE_UNIT);

        g_free (signature);

        result = g_svariant_sub_checked (container, helper, 0, end);
        g_svhelper_unref (helper);

        return result;
      }

    default:
      g_assert_not_reached ();
  }

  g_error ("Attempt to access item %d in a container with only %d items",
           index, g_svariant_n_children (container));
}

void
g_svariant_serialise (GSVariant        container,
                      GSVariantFiller  gsv_filler,
                      gpointer        *children,
                      gsize            n_children)
{
  switch (g_svhelper_type (container.helper))
  {
    case G_SIGNATURE_TYPE_VARIANT:
      {
        GSVariant child = { NULL, container.data, 0 };

        g_assert (n_children == 1);

        /* the child */
        gsv_filler (&child, children[0]);

        /* separator byte */
        container.data[child.size] = '\0';

        /* the signature */
        memcpy (&container.data[child.size + 1],
                g_svhelper_signature (child.helper),
                container.size - child.size - 1);

        /* make sure we got that right... */
        g_assert_cmpint (container.size - child.size - 1, ==,
          g_signature_length (g_svhelper_signature (child.helper)));

        return;
      }

    case G_SIGNATURE_TYPE_DICT_ENTRY:
    case G_SIGNATURE_TYPE_STRUCT:
      {
        gsize n_members;
        gsize offset;
        gsize index;
        gsize i;

        offset = 0;
        index = 0;

        n_members = g_svhelper_n_members (container.helper);

        g_assert_cmpint (n_members, ==, n_children);
        for (i = 0; i < n_members; i++)
          {
            MemberInfo member;
            guint alignment;
            gssize size;

            g_svhelper_member_info (container.helper, i, &member);
            g_svhelper_info (member.helper, &alignment, &size);

            if (size < 0)
              {
                GSVariant child = { NULL, container.data, 0 };

                /* due to the alignment padding, this might be out of range
                 * in that case that we're writing zero bytes, but since
                 * we're writing zero bytes, that's ok...
                 */
                child.data += offset + ((-offset) & alignment);

                gsv_filler (&child, children[i]);

                g_assert (child.helper == member.helper);

                if (child.size)
                  {
                    /* child was non-zero sized.  add padding. */
                    g_svariant_pad (container, &offset, alignment);
                    g_assert (child.data == &container.data[offset]);
                    offset += child.size;
                  }
                else
                  /* if size is zero then we neither align nor pad */
                  ;

                if (member.size == STRUCT_MEMBER_VARIABLE)
                  g_svariant_assign (container, index++, offset);
              }
            else if (size > 0)
              {
                GSVariant child = {};

                g_svariant_pad (container, &offset, alignment);
                child.data = &container.data[offset];
                gsv_filler (&child, children[i]);
                offset += size;

                g_assert (child.helper == member.helper);
                g_assert (child.size == size);
              }
            else
              /* if size is zero then we neither align nor pad */
              ;
          }

        /* pad fixed-size structures */
        {
          guint alignment;
          gssize size;

          g_svhelper_info (container.helper, &alignment, &size);

          if (size > 0)
            {
              g_svariant_pad (container, &offset, alignment);
              g_assert_cmpint (offset, ==, size);
            }
        }

        g_variant_serialiser_sanity_check (container, offset, index);

        return;
      }

    case G_SIGNATURE_TYPE_ARRAY:
      {
        gssize fixed_size;
        guint alignment;
        gsize i;

        g_svhelper_element_info (container.helper,
                                         &alignment, &fixed_size);

        if (container.size == 0)
          {
            g_assert_cmpint (n_children, ==, 0);
            return;
          }

        if (fixed_size > 0)
          {
            for (i = 0; i < n_children; i++)
              {
                GSVariant child = {};

                child.data = &container.data[fixed_size * i];
                gsv_filler (&child, children[i]);

                g_assert (child.size == fixed_size);
              }
          }
        else
          {
            gsize offset;
            gsize index;

            index = n_children;
            offset = 0;

            for (i = 0; i < n_children; i++)
              {
                GSVariant child = {};

                child.data = container.data + offset + ((-offset) & alignment);
                gsv_filler (&child, children[i]);

                if (child.size)
                  {
                    g_svariant_pad (container, &offset, alignment);
                    g_assert (child.data == &container.data[offset]);
                    offset += child.size;
                  }

                g_svariant_assign (container, --index, offset);
              }

            g_assert_cmpint (index, ==, 0);
            g_variant_serialiser_sanity_check (container, offset, n_children);
          }

        return;
      }

    case G_SIGNATURE_TYPE_MAYBE:
      {
        if (container.size == 0)
          {
            g_assert_cmpint (n_children, ==, 0);
          }
        else
          {
            gssize fixed_size;
            GSVariant child = {};

            g_assert_cmpint (n_children, ==, 1);

            child.data = container.data;
            gsv_filler (&child, children[0]);

            g_svhelper_element_info (container.helper,
                                             NULL, &fixed_size);

            if (fixed_size >= 0)
              g_assert_cmpint (fixed_size, ==, child.size);

            if (fixed_size <= 0)
              container.data[child.size++] = '\0';

            g_assert_cmpint (child.size, ==, container.size);
          }

        return;
      }

    default:
      g_assert_not_reached ();
  }
}

gsize
g_svariant_needed_size (GSVHelper       *helper,
                        GSVariantFiller  gsv_filler,
                        gpointer        *children,
                        gsize            n_children)
{
  switch (g_svhelper_type (helper))
  {
    case G_SIGNATURE_TYPE_VARIANT:
      {
        GSVariant child = {};

        g_assert_cmpint (n_children, ==, 1);
        gsv_filler (&child, children[0]);

        return child.size + 1 +
               g_signature_length (g_svhelper_signature (child.helper));
      }

    case G_SIGNATURE_TYPE_ARRAY:
      {
        GSVHelper *elemhelper;
        gssize fixed_size;
        guint alignment;

        if (n_children == 0)
          return 0;

        elemhelper = g_svhelper_element (helper);
        g_svhelper_info (elemhelper, &alignment, &fixed_size);

        if (fixed_size <= 0)
          {
            gsize offset;
            gsize i;

            offset = 0;

            for (i = 0; i < n_children; i++)
              {
                GSVariant child = {};

                gsv_filler (&child, children[i]);
                g_assert (child.helper == elemhelper);

                offset += (-offset) & alignment;
                offset += child.size;
              }

            return g_svariant_determine_size (offset, n_children, TRUE);
          }
        else
          return fixed_size * n_children;
      }

    case G_SIGNATURE_TYPE_MAYBE:
      {
        GSVHelper *elemhelper;
        GSVariant child = {};
        gssize fixed_size;

        g_assert_cmpint (n_children, <=, 1);

        if (n_children == 0)
          return 0;

        elemhelper = g_svhelper_element (helper);
        g_svhelper_info (elemhelper, NULL, &fixed_size);
        gsv_filler (&child, children[0]);
        g_assert (child.helper == elemhelper);

        if (fixed_size > 0)
          return fixed_size;
        else
          return child.size + 1;
      }

    case G_SIGNATURE_TYPE_STRUCT:
    case G_SIGNATURE_TYPE_DICT_ENTRY:

      {
        gsize n_offsets;
        gsize offset;
        gsize i;

        g_assert_cmpint (g_svhelper_n_members (helper), ==, n_children);

        {
          gssize fixed_size;

          g_svhelper_info (helper, NULL, &fixed_size);

          if (fixed_size >= 0)
            return fixed_size;
        }

        n_offsets = 0;
        offset = 0;

        for (i = 0; i < n_children; i++)
          {
            GSVariant child = {};
            MemberInfo info;
            guint alignment;
            gssize size;

            g_svhelper_member_info (helper, i, &info);
            gsv_filler (&child, children[i]);
            g_assert (child.helper == info.helper);

            g_svhelper_info (info.helper, &alignment, &size);

            if (size < 0)
              {
                if (child.size)
                  {
                    offset += (-offset) & alignment;
                    offset += child.size;
                  }

                if (info.size == STRUCT_MEMBER_VARIABLE)
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

        return g_svariant_determine_size (offset, n_offsets, FALSE);
      }

    default:
      g_assert_not_reached ();
  }
}

void
g_svariant_byteswap (GSVariant value)
{
  gssize fixed_size;
  guint alignment;

  if (!value.data)
    return;

  /* the types we potentially need to byteswap are
   * exactly those with alignment requirements.
   */
  g_svhelper_info (value.helper, &alignment, &fixed_size);
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

      children = g_svariant_n_children (value);
      for (i = 0; i < children; i++)
        {
          GSVariant child;

          child = g_svariant_get_child (value, i);
          g_svariant_byteswap (child);
        }
    }
}

void
g_svariant_assert_invariant (GSVariant value)
{
  gssize fixed_size;
  guint alignment;

  g_assert (value.helper != NULL);
  g_assert_cmpint ((value.data == NULL), <=, (value.size == 0));

  g_svhelper_info (value.helper, &alignment, &fixed_size);

  g_assert_cmpint (((gsize) value.data) & alignment, ==, 0);
  if (fixed_size >= 0)
    g_assert_cmpint (value.size, ==, fixed_size);
}

gboolean
g_svariant_is_normalised (GSVariant value)
{
  switch (g_svhelper_type (value.helper))
  {
    case G_SIGNATURE_TYPE_BYTE:
    case G_SIGNATURE_TYPE_INT16:
    case G_SIGNATURE_TYPE_UINT16:
    case G_SIGNATURE_TYPE_INT32:
    case G_SIGNATURE_TYPE_UINT32:
    case G_SIGNATURE_TYPE_INT64:
    case G_SIGNATURE_TYPE_UINT64:
    case G_SIGNATURE_TYPE_DOUBLE:
      return value.data != NULL;

    case G_SIGNATURE_TYPE_BOOLEAN:
      return value.data && (value.data[0] == FALSE ||
                            value.data[0] == TRUE);

    case G_SIGNATURE_TYPE_SIGNATURE:
    case G_SIGNATURE_TYPE_OBJECT_PATH:
      g_error ("don't validate these yet");

    case G_SIGNATURE_TYPE_STRING:
      if (value.size == 0)
        return FALSE;

      if (value.data[value.size - 1])
        return FALSE;

      return strlen ((const char *) value.data) + 1 != value.size;

    case G_SIGNATURE_TYPE_ARRAY:
    case G_SIGNATURE_TYPE_MAYBE:
    case G_SIGNATURE_TYPE_STRUCT:
    case G_SIGNATURE_TYPE_DICT_ENTRY:
    case G_SIGNATURE_TYPE_VARIANT:
      {
        gsize children, i;

        children = g_svariant_n_children (value);
        for (i = 0; i < children; i++)
          {
            GSVariant child;

            child = g_svariant_get_child (value, i);
            if (!g_svariant_is_normalised (child))
              return FALSE;
          }
        break;
      }

    default:
      g_assert_not_reached ();
  }

  return TRUE;
}
