/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#include <string.h>
#include <glib.h>

#include "gvariant-private.h"

typedef struct
{
  GVariant *value;
  int length;
  int offset;
} GVariantIterReal;

/**
 * g_variant_iter_cancel:
 * @iter: a #GVariantIter
 *
 * Causes @iter to drop its reference to the
 * container that it was created with.  You need
 * to call cell this on an iter if for some reason
 * you stopped iterating before reaching the end.
 *
 * You do not need to call this in the normal case
 * of visiting all of the elements.  In this case,
 * the reference will be automatically dropped by
 * g_variant_iter_next() just before it returns
 * %NULL.
 **/

void
g_variant_iter_cancel (GVariantIter *iter)
{
  GVariantIterReal *real = (GVariantIterReal *) iter;

  g_variant_unref (real->value);
  real->value = NULL;
}

/**
 * g_variant_iter_init:
 * @iter: a #GVariantIter
 *
 * Initialises the fields of a #GVariantIter and
 * prepare to iterate over the contents of @value.
 *
 * @iter is allowed to be completely uninitialised
 * prior to this call; it does not, for example,
 * have to be cleared to zeros.
 *
 * After this call, @iter holds a reference to
 * @value.  The reference will only be dropped
 * once all values have been iterated over or by
 * calling g_variant_iter_cancel().
 **/
gint
g_variant_iter_init (GVariantIter *iter,
                     GVariant     *value)
{
  GVariantIterReal *real = (GVariantIterReal *) iter;

  g_assert (sizeof (GVariantIterReal) <= sizeof (GVariantIter));

  real->length = g_variant_n_children (value);
  real->offset = 0;

  if (real->length)
    real->value = g_variant_ref (value);
  else
    real->value = NULL;

  return real->length;
}

/**
 * g_variant_iter_next:
 * @iter: a #GVariantIter
 * @returns: a #GVariant for the next child
 *
 * Retreives the next child value from @iter.  In
 * the event that no more child values exist,
 * %NULL is returned and @iter drops its reference
 * to the value that it was created with.
 **/
GVariant *
g_variant_iter_next (GVariantIter *iter)
{
  GVariantIterReal *real = (GVariantIterReal *) iter;
  GVariant *value;

  if (real->value == NULL)
    return NULL;

  value = g_variant_get_child (real->value, real->offset++);

  if (real->offset == real->length)
    {
      g_variant_unref (real->value);
      real->value = NULL;
    }

  return value;
}

/**
 * g_variant_matches:
 * @value: a #GVariant instance
 * @pattern: a possibly abstract #GVariantType
 * @returns: %TRUE if the type of @value matches @pattern
 *
 * Checks if a value has a type matching the provided pattern.  This
 * call is equivalent to calling g_variant_get_type() then
 * g_variant_type_matches().
 **/
gboolean
g_variant_matches (GVariant           *value,
                   const GVariantType *pattern)
{
  return g_variant_type_matches (g_variant_get_type (value), pattern);
}

/**
 * g_variant_new_boolean:
 * @boolean: a #gboolean value
 * @returns: a new boolean #GVariant instance
 *
 * Creates a new boolean #GVariant instance -- either %TRUE or %FALSE.
 **/
GVariant *
g_variant_new_boolean (gboolean boolean)
{
  guint8 byte = !!boolean;

  return g_variant_new_small (G_VARIANT_TYPE_BOOLEAN, &byte, 1);
}

GVariant *
g_variant_new_byte (guint8 byte)
{
  return g_variant_new_small (G_VARIANT_TYPE_BYTE, &byte, 1);
}

GVariant *
g_variant_new_uint16 (guint16 uint16)
{
  return g_variant_new_small (G_VARIANT_TYPE_UINT16, &uint16, 2);
}

GVariant *
g_variant_new_int16 (gint16 int16)
{
  return g_variant_new_small (G_VARIANT_TYPE_INT16, &int16, 2);
}

GVariant *
g_variant_new_uint32 (guint32 uint32)
{
  return g_variant_new_small (G_VARIANT_TYPE_UINT32, &uint32, 4);
}

GVariant *
g_variant_new_int32 (gint32 int32)
{
  return g_variant_new_small (G_VARIANT_TYPE_INT32, &int32, 4);
}

GVariant *
g_variant_new_uint64 (guint64 uint64)
{
  return g_variant_new_small (G_VARIANT_TYPE_UINT64, &uint64, 8);
}

GVariant *
g_variant_new_int64 (gint64 int64)
{
  return g_variant_new_small (G_VARIANT_TYPE_INT64, &int64, 8);
}

GVariant *
g_variant_new_double (gdouble floating)
{
  return g_variant_new_small (G_VARIANT_TYPE_DOUBLE, &floating, 8);
}

/**
 * g_variant_new_string:
 * @string: a normal C nul-terminated string
 * @returns: a new string #GVariant instance
 *
 * Creates a string #GVariant with the contents of @string.
 **/
GVariant *
g_variant_new_string (const char *string)
{
  return g_variant_load (G_VARIANT_TYPE_STRING, string, strlen (string) + 1, 0);
}

/**
 * g_variant_new_object_path:
 * @string: a normal C nul-terminated string
 * @returns: a new object path #GVariant instance
 *
 * Creates a object path #GVariant with the contents of @string.
 * @string must be a valid DBus object path.
 **/
GVariant *
g_variant_new_object_path (const char *string)
{
  return g_variant_load (G_VARIANT_TYPE_OBJECT_PATH,
                         string, strlen (string) + 1, 0);
}

GVariant *
g_variant_new_signature (const char *string)
{
  return g_variant_load (G_VARIANT_TYPE_SIGNATURE,
                         string, strlen (string) + 1, 0);
}

/**
 * g_variant_new_variant:
 * @value: a #GVariance instance
 * @returns: a new variant #GVariant instance
 *
 * Boxes @value.  The result is a #GVariant instance representing a
 * variant containing the original value.
 **/
GVariant *
g_variant_new_variant (GVariant *value)
{
  GVariant **children;

  children = g_slice_new (GVariant *);
  children[0] = value;

  return g_variant_new_tree (G_VARIANT_TYPE_VARIANT,
                             children, 1,
                             g_variant_is_normalised (value));
}

/**
 * g_variant_get_boolean:
 * @value: a boolean #GVariant instance
 * @returns: %TRUE or %FALSE
 *
 * Returns the boolean value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than 'boolean'.
 **/
gboolean
g_variant_get_boolean (GVariant *value)
{
  guint8 byte;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_BOOLEAN));
  g_variant_store (value, &byte);

  return byte;
}

/**
 * g_variant_get_byte:
 * @value: a byte #GVariant instance
 * @returns: a #guchar
 *
 * Returns the byte value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than 'byte'.
 **/
guint8
g_variant_get_byte (GVariant *value)
{
  guint8 byte;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_BYTE));
  g_variant_store (value, &byte);

  return byte;
}

/**
 * g_variant_get_uint16:
 * @value: a uint16 #GVariant instance
 * @returns: a #guint16
 *
 * Returns the 16-bit unsigned integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than 'uint16' -- even 'int16'.
 **/
guint16
g_variant_get_uint16 (GVariant *value)
{
  guint16 uint16;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_UINT16));
  g_variant_store (value, &uint16);

  return uint16;
}

gint16
g_variant_get_int16 (GVariant *value)
{
  gint16 int16;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_INT16));
  g_variant_store (value, &int16);

  return int16;
}

guint32
g_variant_get_uint32 (GVariant *value)
{
  guint32 uint32;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_UINT32));
  g_variant_store (value, &uint32);

  return uint32;
}

gint32
g_variant_get_int32 (GVariant *value)
{
  gint32 int32;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_INT32));
  g_variant_store (value, &int32);

  return int32;
}

guint64
g_variant_get_uint64 (GVariant *value)
{
  guint64 uint64;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_UINT64));
  g_variant_store (value, &uint64);

  return uint64;
}

gint64
g_variant_get_int64 (GVariant *value)
{
  gint64 int64;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_INT64));
  g_variant_store (value, &int64);

  return int64;
}

gdouble
g_variant_get_double (GVariant *value)
{
  gdouble floating;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_DOUBLE));
  g_variant_store (value, &floating);

  return floating;
}

const char *
g_variant_get_string (GVariant *value,
                      gsize    *length)
{
  g_assert (g_variant_matches (value, G_VARIANT_TYPE_STRING) ||
            g_variant_matches (value, G_VARIANT_TYPE_OBJECT_PATH) ||
            g_variant_matches (value, G_VARIANT_TYPE_SIGNATURE));

  if (length)
    *length = g_variant_get_size (value) - 1;

  return g_variant_get_data (value);
}

char *
g_variant_dup_string (GVariant *value,
                      gsize    *length)
{
  int size;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_STRING) ||
            g_variant_matches (value, G_VARIANT_TYPE_OBJECT_PATH) ||
            g_variant_matches (value, G_VARIANT_TYPE_SIGNATURE));

  size = g_variant_get_size (value);

  if (length)
    *length = size - 1;

  return g_memdup (g_variant_get_data (value), size);
}

GVariant *
g_variant_get_variant (GVariant *value)
{
  g_assert (g_variant_matches (value, G_VARIANT_TYPE_VARIANT));

  return g_variant_get_child (value, 0);
}

struct OPAQUE_TYPE__GVariantBuilder
{
  GVariantBuilder *parent;

  GVariantTypeClass class;
  GVariantType *type;
  const GVariantType *expected;

  GVariant **children;
  int children_allocated;
  int offset : 30;
  int has_child : 1;
  int trusted : 1;
};

static void
g_variant_builder_resize (GVariantBuilder *builder,
                          int              new_allocated)
{
  GVariant **new_children;
  int i;

  g_assert_cmpint (builder->offset, <=, new_allocated);

  new_children = g_slice_alloc (sizeof (GVariant *) * new_allocated);

  for (i = 0; i < builder->offset; i++)
    new_children[i] = builder->children[i];

  g_slice_free1 (sizeof (GVariant **) * builder->children_allocated,
                 builder->children);
  builder->children = new_children;
  builder->children_allocated = new_allocated;
}

static gboolean
g_variant_builder_check_add_value (GVariantBuilder  *builder,
                                   GVariant         *value,
                                   GError          **error)
{
  const GVariantType *type;
  GVariantTypeClass class;

  type = g_variant_get_type (value);
  class = g_variant_type_get_class (type);

  return g_variant_builder_check_add (builder, class, type, error);
}

void
g_variant_builder_add_value (GVariantBuilder *builder,
                             GVariant        *value)
{
  GError *error = NULL;

  if G_UNLIKELY (!g_variant_builder_check_add_value (builder, value, &error))
    g_error ("g_variant_builder_add_value: %s", error->message);

  builder->trusted &= g_variant_is_normalised (value);

  if (builder->expected &&
      (builder->class == G_VARIANT_TYPE_CLASS_STRUCT ||
       builder->class == G_VARIANT_TYPE_CLASS_DICT_ENTRY))
    builder->expected = g_variant_type_next (builder->expected);

  if (builder->offset == builder->children_allocated)
    g_variant_builder_resize (builder, builder->children_allocated * 2);

  builder->children[builder->offset++] = g_variant_ref_sink (value);
}

GVariantBuilder *
g_variant_builder_open (GVariantBuilder    *parent,
                        GVariantTypeClass   class,
                        const GVariantType *type)
{
  GVariantBuilder *child;
  GError *error = NULL;

  if G_UNLIKELY (!g_variant_builder_check_add (parent, class, type, &error))
    g_error ("g_variant_builder_open: %s", error->message);

  if G_UNLIKELY (parent->has_child)
    g_error ("GVariantBuilder already has open child");

  if (class != G_VARIANT_TYPE_CLASS_VARIANT && type == NULL)
    type = parent->expected; /* possibly still NULL */

  child = g_variant_builder_new (class, type);
  parent->has_child = TRUE;
  child->parent = parent;

  return child;
}

GVariantBuilder *
g_variant_builder_close (GVariantBuilder *child)
{
  GVariantBuilder *parent;
  GVariant *value;

  g_assert (child != NULL);
  g_assert (child->parent != NULL);

  parent = child->parent;
  parent->has_child = FALSE;
  parent = child->parent;
  child->parent = NULL;

  value = g_variant_builder_end (child);
  g_variant_builder_add_value (parent, value);

  return parent;
}

GVariantBuilder *
g_variant_builder_new (GVariantTypeClass   class,
                       const GVariantType *type)
{
  GVariantBuilder *builder;

  g_assert (type == NULL || g_variant_type_is_concrete (type));
  g_assert (class == G_VARIANT_TYPE_CLASS_VARIANT ||
            type == NULL || g_variant_type_is_in_class (type, class));

  builder = g_slice_new (GVariantBuilder);
  builder->parent = NULL;
  builder->offset = 0;
  builder->has_child = FALSE;
  builder->class = class;
  builder->type = type ? g_variant_type_copy (type) : NULL;
  builder->expected = NULL;
  builder->trusted = TRUE;

  switch (class)
  {
    case G_VARIANT_TYPE_CLASS_VARIANT:
      builder->children_allocated = 1;
      builder->expected = builder->type;
      break;

    case G_VARIANT_TYPE_CLASS_ARRAY:
      builder->children_allocated = 8;
      if (builder->type)
        builder->expected = g_variant_type_element (builder->type);
      break;

    case G_VARIANT_TYPE_CLASS_MAYBE:
      builder->children_allocated = 1;
      if (builder->type)
        builder->expected = g_variant_type_element (builder->type);
      break;

    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      builder->children_allocated = 2;
      if (builder->type)
        builder->expected = g_variant_type_key (builder->type);
      break;

    case G_VARIANT_TYPE_CLASS_STRUCT:
      builder->children_allocated = 8;
      if (builder->type)
        builder->expected = g_variant_type_first (builder->type);
      break;

    default:
      g_error ("g_variant_builder_new() works only with container types");
   }

  builder->children = g_slice_alloc (sizeof (GVariant *) *
                                     builder->children_allocated);

  return builder;
}

GVariant *
g_variant_builder_end (GVariantBuilder *builder)
{
  GVariantType *my_type;
  GError *error = NULL;
  GVariant *value;

  g_assert (builder->parent == NULL);

  if G_UNLIKELY (!g_variant_builder_check_end (builder, &error))
    g_error ("g_variant_builder_end: %s", error->message);

  g_variant_builder_resize (builder, builder->offset);

  if (builder->class == G_VARIANT_TYPE_CLASS_VARIANT)
    {
      my_type = g_variant_type_copy (G_VARIANT_TYPE_VARIANT);
      if (builder->type)
        g_variant_type_free (builder->type);
    }
  else
    my_type = builder->type;

  if (my_type == NULL)
    switch (builder->class)
    {
      case G_VARIANT_TYPE_CLASS_ARRAY:
        my_type = g_variant_type_new_array (
                    g_variant_get_type (builder->children[0]));
        break;

      case G_VARIANT_TYPE_CLASS_MAYBE:
        my_type = g_variant_type_new_maybe (
                    g_variant_get_type (builder->children[0]));
        break;

      case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
        my_type = g_variant_type_new_dict_entry (
                    g_variant_get_type (builder->children[0]),
                    g_variant_get_type (builder->children[1]));
        break;

      case G_VARIANT_TYPE_CLASS_STRUCT:
        my_type = g_variant_type_new_struct (builder->children,
                                             g_variant_get_type,
                                             builder->offset);
        break;

      default:
        g_assert_not_reached ();
    }

  value = g_variant_new_tree (my_type, builder->children,
                              builder->offset, builder->trusted);

  g_slice_free (GVariantBuilder, builder);
  g_variant_type_free (my_type);

  return value;
}

gboolean
g_variant_builder_check_end (GVariantBuilder     *builder,
                             GError             **error)
{
  g_assert (builder != NULL);
  g_assert (builder->has_child == FALSE);

  switch (builder->class)
  {
    case G_VARIANT_TYPE_CLASS_VARIANT:
      if (builder->offset < 1)
        {
          g_set_error (error, 0, 0,
                       "a variant must contain exactly one value");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_ARRAY:
      if (builder->type == NULL && builder->offset == 0)
        {
          g_set_error (error, 0, 0,
                       "unable to infer type for empty array");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_MAYBE:
      if (builder->type == NULL && builder->offset == 0)
        {
          g_set_error (error, 0, 0,
                       "unable to infer type for maybe with no value");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      if (builder->offset < 2)
        {
          g_set_error (error, 0, 0,
                       "a dictionary entry must have a key and a value");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_STRUCT:
      if (builder->expected)
        {
          gchar *type_string;

          type_string = g_variant_type_dup_string (builder->type);
          g_set_error (error, 0, 0,
                       "a structure of type %s must contain %d children "
                       "but only %d have been given", type_string,
                       g_variant_type_n_items (builder->type),
                       builder->offset);
          g_free (type_string);

          return FALSE;
        }
      break;

    default:
      g_assert_not_reached ();
  }

  return TRUE;
}

gboolean
g_variant_builder_check_add (GVariantBuilder     *builder,
                             GVariantTypeClass    class,
                             const GVariantType  *type,
                             GError             **error)
{
  g_assert (builder != NULL);
  g_assert (builder->has_child == FALSE);
  g_assert (class != G_VARIANT_TYPE_CLASS_INVALID);

  if (class == G_VARIANT_TYPE_CLASS_VARIANT)
    type = NULL;

  if (type && !g_variant_type_is_concrete (type))
    {
      gchar *type_str;

      type_str = g_variant_type_dup_string (type);
      g_set_error (error, G_VARIANT_BUILDER_ERROR,
                   G_VARIANT_BUILDER_ERROR_TYPE,
                   "type '%s' is not a concrete type", type_str);
      g_free (type_str);
      return FALSE;
    }

  if (type && g_variant_type_get_class (type) != class)
    {
      gchar *type_str;

      type_str = g_variant_type_dup_string (type);
      g_set_error (error, G_VARIANT_BUILDER_ERROR,
                   G_VARIANT_BUILDER_ERROR_TYPE,
                   "type '%s' is not of the correct class", type_str);
      g_free (type_str);
      return FALSE;
    }
  /* we now know that class is the natural class of a concrete type */

  if (builder->expected &&
      !g_variant_type_is_in_class (builder->expected, class))
    {
      g_set_error (error, G_VARIANT_BUILDER_ERROR,
                   G_VARIANT_BUILDER_ERROR_TYPE,
                   "expecting value of class '%c', not '%c'",
                   g_variant_type_get_class (builder->expected),
                   class);
      return FALSE;
    }

  if (builder->expected && type &&
      !g_variant_type_matches (type, builder->expected))
    {
      gchar *expected_str, *type_str;

      expected_str = g_variant_type_dup_string (builder->expected);
      type_str = g_variant_type_dup_string (type);
      g_set_error (error, G_VARIANT_BUILDER_ERROR,
                   G_VARIANT_BUILDER_ERROR_TYPE,
                   "type '%s' does not match expected type '%s'",
                   type_str, expected_str);
      g_free (expected_str);
      g_free (type_str);
      return FALSE;
    }

  switch (builder->class)
  {
    case G_VARIANT_TYPE_CLASS_VARIANT:
      if (builder->offset)
        {
          g_set_error (error, G_VARIANT_BUILDER_ERROR,
                       G_VARIANT_BUILDER_ERROR_TOO_MANY,
                       "a variant cannot contain more than one value");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_ARRAY:
      if (builder->expected == NULL && type && builder->offset &&
          !g_variant_matches (builder->children[0], type))
        /* builder type not explicitly specified, but the array has
         * one item in it already, so the others must match...
         */
        {
          g_set_error (error, G_VARIANT_BUILDER_ERROR,
                       G_VARIANT_BUILDER_ERROR_TYPE,
                       "all items in an array must have the same type");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_MAYBE:
      if (builder->offset)
        {
          g_set_error (error, G_VARIANT_BUILDER_ERROR,
                       G_VARIANT_BUILDER_ERROR_TOO_MANY,
                       "a maybe cannot contain more than one value");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      if (builder->offset > 1)
        {
          g_set_error (error, G_VARIANT_BUILDER_ERROR,
                       G_VARIANT_BUILDER_ERROR_TOO_MANY,
                       "a dictionary entry may have only a key and a value");
          return FALSE;
        }
      else if (builder->offset == 0)
        {
          if (!g_variant_type_class_is_basic (class))
            {
              g_set_error (error, G_VARIANT_BUILDER_ERROR,
                           G_VARIANT_BUILDER_ERROR_TYPE,
                           "dictionary entry key must be a basic type");
              return FALSE;
            }
        }
      break;

    case G_VARIANT_TYPE_CLASS_STRUCT:
      if (builder->type && builder->expected == NULL)
        {
          gchar *type_str;

          type_str = g_variant_type_dup_string (builder->type);
          g_set_error (error, G_VARIANT_BUILDER_ERROR,
                       G_VARIANT_BUILDER_ERROR_TOO_MANY,
                       "too many items (%d) for this structure type '%s'",
                       builder->offset + 1, type_str);
          g_free (type_str);

          return FALSE;
        }
      break;

    default:
      g_assert_not_reached ();
  }

  return TRUE;
}

void
g_variant_builder_cancel (GVariantBuilder *builder)
{
  GVariantBuilder *parent;

  g_assert (builder != NULL);

  do
    {
      gsize i;

      for (i = 0; i < builder->offset; i++)
        g_variant_unref (builder->children[i]);

      g_slice_free1 (sizeof (GVariant *) * builder->children_allocated,
                     builder->children);

      if (builder->type)
        g_variant_type_free (builder->type);

      parent = builder->parent;
      g_slice_free (GVariantBuilder, builder);
    }
  while ((builder = parent));
}

void
g_variant_flatten (GVariant *value)
{
  g_variant_get_data (value);
}

const gchar *
g_variant_get_type_string (GVariant *value)
{
  return (const gchar *) g_variant_get_type (value);
}

GVariantTypeClass
g_variant_get_type_class (GVariant *value)
{
  return g_variant_type_get_class (g_variant_get_type (value));
}
