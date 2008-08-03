/*
 * Copyright © 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#include "gvarianttypeinfo.h"
#include <glib.h>

struct OPAQUE_TYPE__GVariantTypeInfo
{
  GVariantType *type;

  gssize fixed_size;
  guchar info_class;
  guchar alignment;
  gint ref_count;
};

typedef struct
{
  GVariantTypeInfo self;

  GVariantTypeInfo *element;
} ArrayInfo;

typedef struct
{
  GVariantTypeInfo self;

  GVariantMemberInfo *members;
  gsize n_members;
} StructInfo;

/* == query == */
const GVariantType *
g_variant_type_info_get_type (GVariantTypeInfo *info)
{
  g_assert_cmpint (info->ref_count, >, 0);

  return info->type;
}

GVariantTypeClass
g_variant_type_info_get_type_class (GVariantTypeInfo *info)
{
  g_assert_cmpint (info->ref_count, >, 0);

  return g_variant_type_get_class (info->type);
}

void
g_variant_type_info_query (GVariantTypeInfo *info,
                           guint            *alignment,
                          gssize           *fixed_size)
{
  g_assert_cmpint (info->ref_count, >, 0);

  if (alignment)
    *alignment = info->alignment;

  if (fixed_size)
    *fixed_size = info->fixed_size;
}

/* == array == */
#define ARRAY_INFO_CLASS 'a'
static ArrayInfo *
ARRAY_INFO (GVariantTypeInfo *info)
{
  g_assert (info->info_class == ARRAY_INFO_CLASS);
  return (ArrayInfo *) info;
}

static void
array_info_free (GVariantTypeInfo *info)
{
  ArrayInfo *array_info = ARRAY_INFO (info);

  g_variant_type_info_unref (array_info->element);
  g_slice_free (ArrayInfo, array_info);
}

static GVariantTypeInfo *
array_info_new (const GVariantType *type)
{
  ArrayInfo *info;

  info = g_slice_new (ArrayInfo);
  info->self.info_class = ARRAY_INFO_CLASS;

  info->element = g_variant_type_info_get (g_variant_type_element (type));
  info->self.alignment = info->element->alignment;
  info->self.fixed_size = -1;

  return (GVariantTypeInfo *) info;
}

GVariantTypeInfo *
g_variant_type_info_element (GVariantTypeInfo *info)
{
  return ARRAY_INFO (info)->element;
}

void
g_variant_type_info_query_element (GVariantTypeInfo *info,
                                   guint            *alignment,
                                   gssize           *fixed_size)
{
  g_variant_type_info_query (ARRAY_INFO (info)->element,
                             alignment, fixed_size);
}

/* == structure == */
#define STRUCT_INFO_CLASS 's'
static StructInfo *
STRUCT_INFO (GVariantTypeInfo *info)
{
  g_assert (info->info_class == STRUCT_INFO_CLASS);
  return (StructInfo *) info;
}

static void
struct_info_free (GVariantTypeInfo *info)
{
  StructInfo *struct_info = STRUCT_INFO (info);
  gint i;

  for (i = 0; i < struct_info->n_members; i++)
    g_variant_type_info_unref (struct_info->members[i].type);

  g_slice_free1 (sizeof (GVariantMemberInfo) * struct_info->n_members,
                 struct_info->members);
  g_slice_free (StructInfo, struct_info);
}

static GVariantTypeInfo *
struct_info_new (const GVariantType *type)
{
  StructInfo *info;
  const GVariantType *item;
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

  info = g_slice_new (StructInfo);
  info->self.info_class = STRUCT_INFO_CLASS;

  if (!g_variant_type_is_in_class (type, G_VARIANT_TYPE_CLASS_DICT_ENTRY))
    info->n_members = g_variant_type_n_items (type);
  else
    info->n_members = 2;
  info->members = g_slice_alloc (sizeof (GVariantMemberInfo) *
                                 info->n_members);

  item = g_variant_type_first (type);
  while (item)
    {
      gssize item_fixed_size;
      guint item_alignment;

      info->members[i].type = g_variant_type_info_get (item);
      item_fixed_size = info->members[i].type->fixed_size;
      item_alignment = info->members[i].type->alignment;

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
      info->members[i].index = index;
      info->members[i].plus = before;
      info->members[i].and = ~aligned;
      info->members[i].or = after;

      /* what is the size of the item? */
      if (item_fixed_size < 0)
        {
          /* variable width item */
          info->members[i].size = STRUCT_MEMBER_VARIABLE;

          fixed = FALSE;
          aligned = 0;
          before = 0;
          after = 0;
          index++;
        }
      else
        {
          /* fixed width item */
          info->members[i].size = item_fixed_size;
          after += item_fixed_size;
        }

      item = g_variant_type_next (item);
      i++;
    }

  /* variable-size offset is not stored for last item */
  if (i && info->members[i - 1].size == STRUCT_MEMBER_VARIABLE)
    info->members[i - 1].size = STRUCT_MEMBER_LAST;

  info->self.alignment = alignment;
  if (fixed)
    {
      /* we have not shifted 'after' bits into 'before' here,
       * so we must _add_ since there might be overlap.
       */
      info->self.fixed_size = ((0 + before) & (~aligned)) + after;
      info->self.fixed_size += (-info->self.fixed_size) & alignment;
    }
  else
    info->self.fixed_size = -1;

  g_assert_cmpint (info->n_members, ==, i);

  return (GVariantTypeInfo *) info;
}

gsize
g_variant_type_info_n_members (GVariantTypeInfo *info)
{
  g_assert_cmpint (info->ref_count, >, 0);

  return STRUCT_INFO (info)->n_members;
}

const GVariantMemberInfo *
g_variant_type_info_member_info (GVariantTypeInfo *info,
                                 gsize             index)
{
  StructInfo *struct_info = STRUCT_INFO (info);

  g_assert_cmpint (info->ref_count, >, 0);

  if (index < struct_info->n_members)
    return &struct_info->members[index];

  return NULL;
}

/* == base == */
#define BASE_INFO_CLASS '\0'
static GVariantTypeInfo *
base_info_new (const GVariantTypeClass class)
{
  GVariantTypeInfo *info;

  info = g_slice_new (GVariantTypeInfo);
  info->info_class = BASE_INFO_CLASS;

  switch (class)
  {
    case G_VARIANT_TYPE_CLASS_BOOLEAN:
    case G_VARIANT_TYPE_CLASS_BYTE:
      info->alignment = 1 - 1;
      info->fixed_size = 1;
      break;

    case G_VARIANT_TYPE_CLASS_UINT16:
    case G_VARIANT_TYPE_CLASS_INT16:
      info->alignment = 2 - 1;
      info->fixed_size = 2;
      break;

    case G_VARIANT_TYPE_CLASS_UINT32:
    case G_VARIANT_TYPE_CLASS_INT32:
      info->alignment = 4 - 1;
      info->fixed_size = 4;
      break;

    case G_VARIANT_TYPE_CLASS_UINT64:
    case G_VARIANT_TYPE_CLASS_INT64:
    case G_VARIANT_TYPE_CLASS_DOUBLE:
      info->alignment = 8 - 1;
      info->fixed_size = 8;
      break;

    case G_VARIANT_TYPE_CLASS_VARIANT:
      info->alignment = 8 - 1;
      info->fixed_size = -1;
      break;

    case G_VARIANT_TYPE_CLASS_STRING:
    case G_VARIANT_TYPE_CLASS_OBJECT_PATH:
    case G_VARIANT_TYPE_CLASS_SIGNATURE:
      info->alignment = 1 - 1;
      info->fixed_size = -1;
      break;

    default:
      g_error ("GVariantTypeInfo: not a valid type character: '%c'", class);
  }

  return info;
}

static void
base_info_free (GVariantTypeInfo *info)
{
  g_slice_free (GVariantTypeInfo, info);
}

/* == new/ref/unref == */
static GStaticRecMutex g_variant_type_info_lock = G_STATIC_REC_MUTEX_INIT;
static GHashTable *g_variant_type_info_table;

GVariantTypeInfo *
g_variant_type_info_get (const GVariantType *type)
{
  GVariantTypeInfo *info;

  if (G_UNLIKELY (g_variant_type_info_table == NULL))
    g_variant_type_info_table = g_hash_table_new (g_variant_type_hash,
                                                  g_variant_type_equal);

  g_static_rec_mutex_lock (&g_variant_type_info_lock);
  info = g_hash_table_lookup (g_variant_type_info_table, type);

  if (info == NULL)
    {
      GVariantTypeClass class;

      class = g_variant_type_get_class (type);

      switch (class)
      {
        case G_VARIANT_TYPE_CLASS_MAYBE:
        case G_VARIANT_TYPE_CLASS_ARRAY:
          info = array_info_new (type);
          break;

        case G_VARIANT_TYPE_CLASS_STRUCT:
        case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
          info = struct_info_new (type);
          break;

        default:
          info = base_info_new (class);
          break;
      }

      info->type = g_variant_type_copy (type);
      g_hash_table_insert (g_variant_type_info_table, info->type, info);
      info->ref_count = 1;
    }
  else
    g_variant_type_info_ref (info);

  g_static_rec_mutex_unlock (&g_variant_type_info_lock);

  return info;
}

GVariantTypeInfo *
g_variant_type_info_ref (GVariantTypeInfo *info)
{
  g_assert_cmpint (info->ref_count, >, 0);
  g_atomic_int_inc (&info->ref_count);

  return info;
}

void
g_variant_type_info_unref (GVariantTypeInfo *info)
{
  g_assert_cmpint (info->ref_count, >, 0);

  if (g_atomic_int_dec_and_test (&info->ref_count))
    {
      g_static_rec_mutex_lock (&g_variant_type_info_lock);
      g_hash_table_remove (g_variant_type_info_table, info->type);
      g_static_rec_mutex_unlock (&g_variant_type_info_lock);

      g_variant_type_free (info->type);

      switch (info->info_class)
      {
        case ARRAY_INFO_CLASS:
          array_info_free (info);
          break;

        case STRUCT_INFO_CLASS:
          struct_info_free (info);
          break;

        case BASE_INFO_CLASS:
          base_info_free (info);
          break;

        default:
          g_error ("GVariantTypeInfo with invalid class '%c'",
                   info->info_class);
      }
    }
}
