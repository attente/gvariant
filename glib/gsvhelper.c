/*
 * Copyright © 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#include "gsvhelper.h"
#include <glib.h>

struct OPAQUE_TYPE_GSVHelper
{
  GSignature signature;
  gssize fixed_size;
  guint alignment;
  gint ref_count;
};

typedef struct
{
  GSVHelper self;

  GSVHelper *element;
} ArrayHelper;

typedef struct
{
  GSVHelper self;

  MemberInfo *members;
  int n_members;
} StructHelper;

GHashTable *g_svhelper_table;

typedef struct
{
  gchar fixed_size;
  gchar alignment;
} GSVHelperBaseTypeInfo;

#define base_type_table_entry(type,size,align) \
  [G_SIGNATURE_TYPE_##type] = {size, (align) - 1}
GSVHelperBaseTypeInfo g_svhelper_base_type_table[] =
{
                      /* type       size  align */
  base_type_table_entry (STRING,      -1, 1),
  base_type_table_entry (OBJECT_PATH, -1, 1),
  base_type_table_entry (SIGNATURE,   -1, 1),
  base_type_table_entry (VARIANT,     -1, 8),

  base_type_table_entry (BOOLEAN,      1, 1),
  base_type_table_entry (BYTE,         1, 1),
  base_type_table_entry (UINT16,       2, 2),
  base_type_table_entry (INT16,        2, 2),
  base_type_table_entry (UINT32,       4, 4),
  base_type_table_entry (INT32,        4, 4),
  base_type_table_entry (UINT64,       8, 8),
  base_type_table_entry (INT64,        8, 8),
  base_type_table_entry (DOUBLE,       8, 8)
};

static GSVHelper *
g_svhelper_table_get (GSignature signature)
{
  GSVHelper *helper;

  if (G_UNLIKELY (g_svhelper_table == NULL))
    g_svhelper_table = g_hash_table_new (g_signature_hash,
                                                 g_signature_equal);

  helper = g_hash_table_lookup (g_svhelper_table, signature);

  if (helper)
    {
      g_assert_cmpint (helper->ref_count, >, 0);
      g_atomic_int_inc (&helper->ref_count);
    }

  return helper;
}

static void
g_svhelper_table_add (GSVHelper *helper)
{
  helper->ref_count = 1;
  g_hash_table_insert (g_svhelper_table,
                       helper->signature, helper);
}

static gboolean
g_svhelper_table_put (GSVHelper *helper)
{
  gboolean last_reference;

  g_assert_cmpint (helper->ref_count, >, 0);
  last_reference = g_atomic_int_dec_and_test (&helper->ref_count);

  if (last_reference)
    g_hash_table_remove (g_svhelper_table, helper->signature);

  return last_reference;
}

GSignatureType
g_svhelper_type (GSVHelper *helper)
{
  g_assert_cmpint (helper->ref_count, >, 0);

  return g_signature_type (helper->signature);
}

GSignature
g_svhelper_signature (GSVHelper *helper)
{
  g_assert_cmpint (helper->ref_count, >, 0);

  return helper->signature;
}

static void
array_helper_free (ArrayHelper *helper)
{
  g_svhelper_unref (helper->element);
  g_slice_free (ArrayHelper, helper);
}

static void
struct_helper_free (StructHelper *helper)
{
  gint i;

  for (i = 0; i < helper->n_members; i++)
    g_svhelper_unref (helper->members[i].helper);

  g_slice_free1 (sizeof (MemberInfo) * helper->n_members, helper->members);
  g_slice_free (StructHelper, helper);
}

static ArrayHelper *
array_helper_new (GSignature signature)
{
  ArrayHelper *helper;
  GSignature element;

  helper = g_slice_new (ArrayHelper);
  element = g_signature_element (signature);
  helper->element = g_svhelper_get (element);

  helper->self.alignment = helper->element->alignment;
  helper->self.fixed_size = -1;

  return helper;
}

static StructHelper *
struct_helper_new (GSignature signature)
{
  StructHelper *helper;
  GSignature item;
  guint alignment;
  gboolean fixed;
  guint aligned;
  gsize before;
  gsize after;
  gsize index;
  gint i;

  alignment = 0;
  fixed = TRUE;

  aligned = 0;
  before = 0;
  after = 0;

  index = -1;
  i = 0;

  helper = g_slice_new (StructHelper);

  if (!g_signature_has_type (signature, G_SIGNATURE_TYPE_DICT_ENTRY))
    helper->n_members = g_signature_items (signature);
  else
    helper->n_members = 2;
  helper->members = g_slice_alloc (sizeof (MemberInfo) * helper->n_members);

  item = g_signature_first (signature);
  while (item)
    {
      gssize item_fixed_size;
      guint item_alignment;

      helper->members[i].helper = g_svhelper_get (item);
      item_fixed_size = helper->members[i].helper->fixed_size;
      item_alignment = helper->members[i].helper->alignment;

      alignment |= item_alignment;

      /* align for the start of the item */
      if (item_alignment > aligned)
        {
          before += after + ((-after) & aligned) + item_alignment;
          aligned = item_alignment;
          after = 0;
        }
      else
        after += (-after) & item_alignment;

      /* from a mathematical standpoint, the following two lines do
       * exactly nothing.  they ensure that 'after' is always less
       * than 8, however, which means that storing it uses less
       * memory.
       *
       * ultimately we will perform the following calculation:
       *
       *    (n + before) & ~aligned + after     (for some 'n')
       *
       * equivalently, we can shift the high order bits from 'after'
       * into 'before' and perform the same calculation with the same
       * result:
       *
       *    (n + before') & ~aligned + after'
       *      where:
       *        before' = after & ~aligned
       *        after' = after & aligned
       *
       * which because after' only contains bits not in ~aligned, is
       * actually the same as:
       *
       *    (n + before') & ~aligned | after'
       */
      before += after & ~aligned;
      after &= aligned;

      /* store the starting information */
      helper->members[i].index = index;
      helper->members[i].plus = before;
      helper->members[i].and = ~aligned;
      helper->members[i].or = after;

      /* what is the size of the item? */
      if (item_fixed_size < 0)
        {
          /* variable width item */
          helper->members[i].size = STRUCT_MEMBER_VARIABLE;

          fixed = FALSE;
          aligned = 0;
          before = 0;
          after = 0;
          index++;
        }
      else
        {
          /* fixed width item */
          helper->members[i].size = item_fixed_size;
          after += item_fixed_size;
        }

      item = g_signature_next (item);
      i++;
    }

  /* variable-size offset is not stored for last item */
  if (i && helper->members[i - 1].size == STRUCT_MEMBER_VARIABLE)
    helper->members[i - 1].size = STRUCT_MEMBER_LAST;

  helper->self.alignment = alignment;
  if (fixed)
    {
      /* we have not shifted 'after' bits into 'before' here,
       * so we must _add_ since there might be overlap.
       */
      helper->self.fixed_size = ((0 + before) & (~aligned)) + after;
      helper->self.fixed_size += (-helper->self.fixed_size) & alignment;
    }
  else
    helper->self.fixed_size = -1;

  g_assert_cmpint (helper->n_members, ==, i);

  return helper;
}


GSVHelper *
g_svhelper_get (GSignature signature)
{
  GSVHelperBaseTypeInfo *info;
  GSignatureType type;
  GSVHelper *helper;

  if ((helper = g_svhelper_table_get (signature)) != NULL)
    return helper;

  type = g_signature_type (signature);
  switch (type)
  {
    case G_SIGNATURE_TYPE_MAYBE:
    case G_SIGNATURE_TYPE_ARRAY:
      helper = (GSVHelper *) array_helper_new (signature);
      break;

    case G_SIGNATURE_TYPE_STRUCT:
    case G_SIGNATURE_TYPE_DICT_ENTRY:
      helper = (GSVHelper *) struct_helper_new (signature);
      break;

    default:
      helper = g_slice_new (GSVHelper);

      g_assert_cmpint (type, <, G_N_ELEMENTS (g_svhelper_base_type_table));
      info = &g_svhelper_base_type_table[type];

      helper->alignment = info->alignment;
      helper->fixed_size = info->fixed_size;
  }

  helper->signature = g_signature_copy (signature);
  g_svhelper_table_add (helper);

  return helper;
}

GSVHelper *
g_svhelper_ref (GSVHelper *helper)
{
  g_assert_cmpint (helper->ref_count, >, 0);
  g_atomic_int_inc (&helper->ref_count);

  return helper;
}

void
g_svhelper_unref (GSVHelper *helper)
{
  g_assert_cmpint (helper->ref_count, >, 0);

  if (g_svhelper_table_put (helper))
    {
      GSignatureType type;

      type = g_signature_type (helper->signature);
      g_signature_free (helper->signature);

      switch (type)
      {
        case G_SIGNATURE_TYPE_MAYBE:
        case G_SIGNATURE_TYPE_ARRAY:
          array_helper_free ((ArrayHelper *) helper);
          break;

        case G_SIGNATURE_TYPE_STRUCT:
        case G_SIGNATURE_TYPE_DICT_ENTRY:
          struct_helper_free ((StructHelper *) helper);
          break;

        default:
          g_slice_free (GSVHelper, helper);
          break;
      }
    }
}

void
g_svhelper_info (GSVHelper *helper,
                 guint     *alignment,
                 gssize    *fixed_size)
{
  g_assert_cmpint (helper->ref_count, >, 0);

  if (alignment)
    *alignment = helper->alignment;

  if (fixed_size)
    *fixed_size = helper->fixed_size;
}

gsize
g_svhelper_n_members (GSVHelper *helper)
{
  StructHelper *struct_helper;

  g_assert_cmpint (helper->ref_count, >, 0);

  if (g_signature_type (helper->signature) == G_SIGNATURE_TYPE_DICT_ENTRY)
    g_assert (((StructHelper *)helper)->n_members == 2);
  else
    g_assert (g_signature_type (helper->signature) == G_SIGNATURE_TYPE_STRUCT);

  struct_helper = (StructHelper *) helper;

  return struct_helper->n_members;
}

gboolean
g_svhelper_member_info (GSVHelper  *helper,
                        gsize       index,
                        MemberInfo *info)
{
  StructHelper *struct_helper;

  g_assert_cmpint (helper->ref_count, >, 0);
  g_assert (g_signature_type (helper->signature) == G_SIGNATURE_TYPE_STRUCT ||
            g_signature_type (helper->signature) == G_SIGNATURE_TYPE_DICT_ENTRY);

  struct_helper = (StructHelper *) helper;

  if (index < struct_helper->n_members)
    {
      *info = struct_helper->members[index];
      return TRUE;
    }
  else
    return FALSE;
}

GSVHelper *
g_svhelper_element (GSVHelper *helper)
{
  ArrayHelper *array_helper;

  g_assert_cmpint (helper->ref_count, >, 0);
  g_assert (g_signature_type (helper->signature) == G_SIGNATURE_TYPE_ARRAY ||
            g_signature_type (helper->signature) == G_SIGNATURE_TYPE_MAYBE);
  array_helper = (ArrayHelper *) helper;

  return array_helper->element;
}

void
g_svhelper_element_info (GSVHelper *helper,
                         guint     *alignment,
                         gssize    *fixed_size)
{
  GSVHelper *element;

  element = g_svhelper_element (helper);
  g_svhelper_info (element, alignment, fixed_size);
}

static gboolean
g_svhelper_scan_internal (const guchar *data,
                          gsize        *size)
{
  /* need at least one signature byte */
  if (*size < 1)
    return FALSE;

  switch (data[--*size])
  {
    case ')':
      /* consume zero or more item signatures */
      while (g_svhelper_scan (data, size));

      /* make sure we started with '(' */
      return *size > 0 && data[--*size] == '(';

    case '}':
      /* value */
      if (!g_svhelper_scan (data, size))
        return -1;

      if (*size < 1)
        return FALSE;

      /* key */
      switch (data[--*size])
      {
        /* only basic types may be keys */
        case 'y': case 'i': case 'n': case 't': case 'x': case 'u':
        case 'q': case 's': case 'o': case 'g': case 'd': case 'b':
          break;

        default:
          return -1;
      }

      /* make sure we started with '{' */
      return *size > 0 && data[--*size] == '{';

    case 'y': case 'i': case 'n': case 't': case 'x': case 'u':
    case 'q': case 's': case 'o': case 'g': case 'd': case 'b':
    case 'v':
      break;

    default:
      /* this is not the end of a signature */
      return FALSE;
  }

  while (*size > 0 && (data[*size] == 'a' || data[*size] == 'm'))
    --*size;

  return TRUE;
}

GSVHelper *
g_svhelper_scan (const guchar *data,
                 gsize        *size)
{
  gsize end;

  end = *size;

  if (!g_svhelper_scan_internal (data, &end))
    return NULL;

  if (end == 0)
    return NULL;

  if (data[end - 1] != '\0')
    return NULL;

  *size = end - 1;
  return g_svhelper_get ((GSignature) &data[end]);
}

gboolean
g_svhelper_equal (GSVHelper *one,
                  GSVHelper *two)
{
  return one == two;
}
