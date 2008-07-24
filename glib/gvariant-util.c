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

  real->value = g_variant_unref (real->value);
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
    real->value = g_variant_unref (real->value);

  return value;
}

/**
 * g_variant_iterate:
 * @iter: a #GVariantIter
 * @signature_string: a #GSignature string
 * @returns: %TRUE if a child was fetched or
 *   %FALSE if no children remain.
 *
 * Retreives the next child value from @iter and
 * deconstructs it according to @signature_string.
 *
 * This call is essentially equivalent to
 * g_variant_iter_next() and g_variant_get().  On
 * all but the first access to the iterator the
 * given pointers are freed, if appropriate.
 *
 * It might be used as follows:
 *
 * <programlisting>
 * {
 *   GVariantIter iter;
 *   char *key, *value;
 *   ...
 *
 *   while (g_variant_iterate (iter, "{ss}", &key, &value))
 *     printf ("dict{%s} = %s\n", key, value);
 * }
 * </programlisting>
 *
 * Note that on each time through the loop 'key'
 * and 'value' are newly allocated strings.  They
 * do not need to be freed, however, since the
 * next call to g_variant_iterate will free them.
 *
 * Before the first iteration of the loop, the
 * pointers are not expected to be initialised to
 * any particular value.  After the last iteration
 * the pointers will be set to %NULL.
 *
 * If you wish to 'steal' the newly allocated
 * strings for yourself then you must set the
 * pointers to %NULL before the next call to
 * g_variant_iterate() to prevent them from being
 * freed.
 **/
gboolean
g_variant_iterate (GVariantIter *iter,
                   const char   *signature_string,
                   ...)
{
  GVariant *value;
  va_list ap;

  value = g_variant_iter_next (iter);

  if (value == NULL)
    return FALSE;

  va_start (ap, signature_string);
  g_variant_vget (value, TRUE, g_signature (signature_string), ap);
  va_end (ap);

  g_variant_unref (value);

  return TRUE;
}

/**
 * g_variant_matches:
 * @value: a #GVariant instance
 * @pattern: a possibly abstract #GSignature
 * @returns: %TRUE if the type of @value matches @pattern
 *
 * Checks if a value has a type matching the provided pattern.  This
 * call is equivalent to calling g_variant_get_signature() then
 * g_signature_matches().
 **/
gboolean
g_variant_matches (GVariant   *value,
                   GSignature  pattern)
{
  return g_signature_matches (g_variant_get_signature (value), pattern);
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

  return g_variant_new_small (G_SIGNATURE_BOOLEAN, &byte, 1);
}

GVariant *
g_variant_new_byte (guint8 byte)
{
  return g_variant_new_small (G_SIGNATURE_BYTE, &byte, 1);
}

GVariant *
g_variant_new_uint16 (guint16 uint16)
{
  return g_variant_new_small (G_SIGNATURE_UINT16, &uint16, 2);
}

GVariant *
g_variant_new_int16 (gint16 int16)
{
  return g_variant_new_small (G_SIGNATURE_INT16, &int16, 2);
}

GVariant *
g_variant_new_uint32 (guint32 uint32)
{
  return g_variant_new_small (G_SIGNATURE_UINT32, &uint32, 4);
}

GVariant *
g_variant_new_int32 (gint32 int32)
{
  return g_variant_new_small (G_SIGNATURE_INT32, &int32, 4);
}

GVariant *
g_variant_new_uint64 (guint64 uint64)
{
  return g_variant_new_small (G_SIGNATURE_UINT64, &uint64, 8);
}

GVariant *
g_variant_new_int64 (gint64 int64)
{
  return g_variant_new_small (G_SIGNATURE_INT64, &int64, 8);
}

GVariant *
g_variant_new_double (gdouble floating)
{
  return g_variant_new_small (G_SIGNATURE_DOUBLE, &floating, 8);
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
  return g_variant_load (G_SIGNATURE_STRING, string, strlen (string) + 1, 0);
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
  return g_variant_load (G_SIGNATURE_OBJECT_PATH,
                         string, strlen (string) + 1, 0);
}

GVariant *
g_variant_new_signature (const char *string)
{
  return g_variant_load (G_SIGNATURE_SIGNATURE,
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

  return g_variant_new_tree (g_svhelper_get (G_SIGNATURE_VARIANT),
                             children, 1, g_variant_is_normalised (value));
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

  g_assert (g_variant_matches (value, G_SIGNATURE_BOOLEAN));
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

  g_assert (g_variant_matches (value, G_SIGNATURE_BYTE));
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

  g_assert (g_variant_matches (value, G_SIGNATURE_UINT16));
  g_variant_store (value, &uint16);

  return uint16;
}

gint16
g_variant_get_int16 (GVariant *value)
{
  gint16 int16;

  g_assert (g_variant_matches (value, G_SIGNATURE_INT16));
  g_variant_store (value, &int16);

  return int16;
}

guint32
g_variant_get_uint32 (GVariant *value)
{
  guint32 uint32;

  g_assert (g_variant_matches (value, G_SIGNATURE_UINT32));
  g_variant_store (value, &uint32);

  return uint32;
}

gint32
g_variant_get_int32 (GVariant *value)
{
  gint32 int32;

  g_assert (g_variant_matches (value, G_SIGNATURE_INT32));
  g_variant_store (value, &int32);

  return int32;
}

guint64
g_variant_get_uint64 (GVariant *value)
{
  guint64 uint64;

  g_assert (g_variant_matches (value, G_SIGNATURE_UINT64));
  g_variant_store (value, &uint64);

  return uint64;
}

gint64
g_variant_get_int64 (GVariant *value)
{
  gint64 int64;

  g_assert (g_variant_matches (value, G_SIGNATURE_INT64));
  g_variant_store (value, &int64);

  return int64;
}

gdouble
g_variant_get_double (GVariant *value)
{
  gdouble floating;

  g_assert (g_variant_matches (value, G_SIGNATURE_DOUBLE));
  g_variant_store (value, &floating);

  return floating;
}

const char *
g_variant_get_string (GVariant *value,
                      gsize    *length)
{
  g_assert (g_variant_matches (value, G_SIGNATURE_STRING) ||
            g_variant_matches (value, G_SIGNATURE_OBJECT_PATH) ||
            g_variant_matches (value, G_SIGNATURE_SIGNATURE));

  if (length)
    *length = g_variant_get_size (value) - 1;

  return g_variant_get_data (value);
}

char *
g_variant_dup_string (GVariant *value,
                      gsize    *length)
{
  int size;

  g_assert (g_variant_matches (value, G_SIGNATURE_STRING) ||
            g_variant_matches (value, G_SIGNATURE_OBJECT_PATH) ||
            g_variant_matches (value, G_SIGNATURE_SIGNATURE));

  size = g_variant_get_size (value);

  if (length)
    *length = size - 1;

  return g_memdup (g_variant_get_data (value), size);
}

GVariant *
g_variant_get_variant (GVariant *value)
{
  g_assert (g_variant_matches (value, G_SIGNATURE_VARIANT));

  return g_variant_get_child (value, 0);
}

struct OPAQUE_TYPE__GVariantBuilder
{
  GVariantBuilder *parent;

  GSignatureType type;
  GSignature signature;
  GSignature expected;

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

void
g_variant_builder_add_value (GVariantBuilder *builder,
                             GVariant        *value)
{
  GSignature signature = g_variant_get_signature (value);
  GError *error = NULL;

  if (G_UNLIKELY (!g_variant_builder_check_add (builder,
                                                g_signature_type (signature),
                                                signature, &error)))
    g_error ("g_variant_builder_add_value: %s", error->message);

  builder->trusted &= g_variant_is_normalised (value);

  if (builder->signature == NULL)
    {
      if (builder->type == G_SIGNATURE_TYPE_MAYBE)
        {
          builder->signature = g_signature_maybeify (signature);
          builder->expected = g_signature_element (builder->signature);
        }
      else if (builder->type == G_SIGNATURE_TYPE_ARRAY)
        {
          builder->signature = g_signature_arrayify (signature);
          builder->expected = g_signature_element (builder->signature);
        }
    }
  else
    {
      if (builder->type == G_SIGNATURE_TYPE_VARIANT)
        {
          if (builder->expected)
            g_signature_free (builder->expected);
          builder->expected = NULL;
        }
      else if (builder->type == G_SIGNATURE_TYPE_STRUCT ||
               builder->type == G_SIGNATURE_TYPE_DICT_ENTRY)
        builder->expected = g_signature_next (builder->expected);
    }

  if (builder->offset == builder->children_allocated)
    g_variant_builder_resize (builder, builder->children_allocated * 2);

  builder->children[builder->offset++] = g_variant_ref_sink (value);
}

void
g_variant_builder_add (GVariantBuilder *builder,
                       const char      *signature,
                       ...)
{
  GVariant *variant;
  va_list ap;

  va_start (ap, signature);
  variant = g_variant_vnew (g_signature (signature), ap);
  va_end (ap);

  g_variant_builder_add_value (builder, variant);
}

GVariantBuilder *
g_variant_builder_open (GVariantBuilder *parent,
                        GSignatureType   type,
                        GSignature       signature)
{
  GVariantBuilder *child;
  GError *error = NULL;

  if (G_UNLIKELY (!g_variant_builder_check_add (parent, type,
                                                signature, &error)))
    g_error ("g_variant_builder_open: %s", error->message);

  if (type != G_SIGNATURE_TYPE_VARIANT && signature == NULL)
    signature = parent->expected; /* possibly still NULL */

  child = g_variant_builder_new (type, signature);
  parent->has_child = TRUE;
  child->parent = parent;

  return child;
}

GVariantBuilder *
g_variant_builder_close (GVariantBuilder *child)
{
  GVariantBuilder *parent;

  g_assert (child != NULL);
  g_assert (child->parent != NULL);

  parent = child->parent;
  parent->has_child = FALSE;
  parent = child->parent;
  child->parent = NULL;

  g_variant_builder_add_value (parent, g_variant_builder_end (child));

  return parent;
}

GVariantBuilder *
g_variant_builder_new (GSignatureType type,
                       GSignature     signature)
{
  GVariantBuilder *builder;

  g_assert (type != G_SIGNATURE_TYPE_INVALID);
  g_assert (signature == NULL ||
            type == G_SIGNATURE_TYPE_VARIANT ||
            g_signature_type (signature) == type);
  g_assert (signature == NULL ||
            g_signature_concrete (signature));

  builder = g_slice_new (GVariantBuilder);
  builder->parent = NULL;
  builder->offset = 0;
  builder->has_child = FALSE;
  builder->type = type;
  builder->signature = signature ? g_signature_copy (signature) : NULL;
  builder->expected = NULL;
  builder->trusted = TRUE;

  switch (type)
  {
    case G_SIGNATURE_TYPE_VARIANT:
      builder->expected = builder->signature;
      builder->signature = g_signature_copy (G_SIGNATURE_VARIANT);
      builder->children = g_slice_new (GVariant *);
      builder->children_allocated = 1;
      return builder;

    case G_SIGNATURE_TYPE_ARRAY:
      builder->children_allocated = 8;
      break;

    case G_SIGNATURE_TYPE_MAYBE:
      builder->children_allocated = 1;
      break;

    case G_SIGNATURE_TYPE_DICT_ENTRY:
      builder->children_allocated = 2;
      break;

    case G_SIGNATURE_TYPE_STRUCT:
      builder->children_allocated = 8;
      break;

    default:
      g_error ("g_variant_builder_new() works only with container types");
   }
  builder->children = g_slice_alloc (sizeof (GVariant *) *
                                     builder->children_allocated);

  if (signature)
    builder->expected = g_signature_open_blindly (signature);

  return builder;
}

GVariant *
g_variant_builder_end (GVariantBuilder *builder)
{
  GError *error = NULL;
  GSVHelper *helper;
  GVariant *value;

  if (G_UNLIKELY (!g_variant_builder_check_end (builder, &error)))
    g_error ("g_variant_builder_end: %s", error->message);

  g_assert (builder->has_child == FALSE);
  g_assert (builder->parent == NULL);

  g_variant_builder_resize (builder, builder->offset);

  if (builder->signature == NULL)
    {
      GSignature *signatures;
      int i;

      signatures = g_newa (GSignature, builder->offset);

      for (i = 0; i < builder->offset; i++)
        signatures[i] = g_variant_get_signature (builder->children[i]);

      if (builder->type == G_SIGNATURE_TYPE_DICT_ENTRY)
        {
          g_assert (builder->offset == 2);
          builder->signature = g_signature_dictify (signatures[0],
                                                    signatures[1]);
        }
      else if (builder->type == G_SIGNATURE_TYPE_STRUCT)
        {
          builder->signature = g_signature_structify (signatures,
                                                      builder->offset);
        }
      else
        g_error ("signature not set on dict-entry/struct?");
    }
  else
    {
      if (builder->type == G_SIGNATURE_TYPE_VARIANT && builder->expected)
        g_signature_free (builder->expected);
    }

  helper = g_svhelper_get (builder->signature);
  value = g_variant_new_tree (helper, builder->children,
                              builder->offset, builder->trusted);
  g_signature_free (builder->signature);
  g_slice_free (GVariantBuilder, builder);

  return value;
}

gboolean
g_variant_builder_check_end (GVariantBuilder  *builder,
                             GError          **error)
{
  g_assert (builder != NULL);

  switch (builder->type)
  {
    case G_SIGNATURE_TYPE_VARIANT:
      if (builder->offset < 1)
        {
          g_set_error (error, 0, 0,
                       "a variant must contain exactly one value");
          return FALSE;
        }
      break;

    case G_SIGNATURE_TYPE_ARRAY:
      if (builder->signature == NULL)
        {
          g_set_error (error, 0, 0,
                       "unable to infer signature for empty array");
          return FALSE;
        }
      break;

    case G_SIGNATURE_TYPE_MAYBE:
      if (builder->signature == NULL)
        {
          g_set_error (error, 0, 0,
                       "unable to infer signature for maybe with no value");
          return FALSE;
        }
      break;

    case G_SIGNATURE_TYPE_DICT_ENTRY:
      if (builder->offset < 2)
        {
          g_set_error (error, 0, 0,
                       "a dictionary entry must have a key and a value");
          return FALSE;
        }
      break;

    case G_SIGNATURE_TYPE_STRUCT:
      if (builder->expected)
        {
          char *signature_string;

          signature_string = g_signature_get (builder->signature);
          g_set_error (error, 0, 0,
                       "a structure of type %s must contain %d children "
                       "but only %d have been given", signature_string,
                       g_signature_items (builder->signature),
                       builder->offset);
          g_free (signature_string);

          return FALSE;
        }
      break;

    default:
      g_assert_not_reached ();
  }

  return TRUE;
}

gboolean
g_variant_builder_check_add (GVariantBuilder  *builder,
                             GSignatureType    type,
                             GSignature        signature,
                             GError          **error)
{
  g_assert (builder != NULL);
  g_assert (builder->has_child == FALSE);
  g_assert (type != G_SIGNATURE_TYPE_INVALID);

  if (type == G_SIGNATURE_TYPE_VARIANT)
    signature = NULL;

  if (signature && g_signature_type (signature) != type)
    {
      char *signature_str;

      signature_str = g_signature_get (signature);
      g_set_error (error, 0, 0,
                   "signature '%s' is not of correct type", signature_str);
      g_free (signature_str);
      return FALSE;
    }

  if (signature && !g_signature_concrete (signature))
    {
      char *signature_str;

      signature_str = g_signature_get (signature);
      g_set_error (error, 0, 0,
                   "signature '%s' is not concrete", signature_str);
      g_free (signature_str);
      return FALSE;
    }

  if (builder->expected && g_signature_type (builder->expected) != type)
    {
      g_set_error (error, 0, 0,
                   "expecting value of type '%c', not '%c'",
                   g_signature_type (builder->expected), type);
      return FALSE;
    }

  if (builder->expected && signature &&
      !g_signature_equal (builder->expected, signature))
    {
      char *signature_str, *expected_str;

      signature_str = g_signature_get (signature);
      expected_str = g_signature_get (builder->expected);
      g_set_error (error, 0, 0,
                   "signature '%s' does not match expected signature '%s'",
                   signature_str, expected_str);
      g_free (signature_str);
      g_free (expected_str);
      return FALSE;
    }

  switch (builder->type)
  {
    case G_SIGNATURE_TYPE_VARIANT:
      if (builder->offset)
        {
          g_set_error (error, 0, 0,
                       "a variant cannot contain more than one value");
          return FALSE;
        }
      break;

    case G_SIGNATURE_TYPE_ARRAY:
      break;

    case G_SIGNATURE_TYPE_MAYBE:
      if (builder->offset)
        {
          g_set_error (error, 0, 0,
                       "a maybe cannot contain more than one value");
          return FALSE;
        }
      break;

    case G_SIGNATURE_TYPE_DICT_ENTRY:
      if (builder->offset > 1)
        {
          g_set_error (error, 0, 0,
                       "a dictionary entry may have only a key and a value");
          return FALSE;
        }
      else if (builder->offset == 0)
        {
          if (!g_signature_type_is_basic (type))
            {
              g_set_error (error, 0, 0,
                           "dictionary entry key must be a basic type");
              return FALSE;
            }
        }
      break;

    case G_SIGNATURE_TYPE_STRUCT:
      if (builder->signature && builder->expected == NULL)
        {
          char *signature_str;

          signature_str = g_signature_get (builder->signature);
          g_set_error (error, 0, 0,
                       "too many items (%d) for this structure type '%s'",
                       builder->offset + 1, signature_str);
          g_free (signature_str);

          return FALSE;
        }
      break;

    default:
      g_assert_not_reached ();
  }

  return TRUE;
}

void
g_variant_builder_abort (GVariantBuilder *builder)
{
  GVariantBuilder *parent;

  g_assert (builder != NULL);

  do
    {
      int i;

      for (i = 0; i < builder->offset; i++)
        g_variant_unref (builder->children[i]);

      g_slice_free1 (sizeof (GVariant *) * builder->children_allocated,
                     builder->children);

      if (builder->signature)
        g_signature_free (builder->signature);

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
