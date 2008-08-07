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

/**
 * GVariantIter:
 *
 * An opaque structure type used to iterate over a container #GVariant
 * instance.
 *
 * The iter must be initialised with a call to g_variant_iter_init()
 * before using it.  After that, g_variant_iter_next() will return the
 * child values, in order.
 *
 * The iter may maintain a reference to the container #GVariant until
 * g_variant_iter_next() returns %NULL.  For this reason, it is
 * essential that you call g_variant_iter_next() until %NULL is
 * returned.  If you want to abort iterating part way through then use
 * g_variant_iter_cancel().
 */
typedef struct
{
  GVariant *value;
  GVariant *child;
  gsize length;
  gsize offset;
  gboolean cancelled;
} GVariantIterReal;

/**
 * g_variant_iter_init:
 * @iter: a #GVariantIter
 * @value: a container #GVariant instance
 * @returns: the number of items in the container
 *
 * Initialises the fields of a #GVariantIter and perpare to iterate
 * over the contents of @value.
 *
 * @iter is allowed to be completely uninitialised prior to this call;
 * it does not, for example, have to be cleared to zeros.  For this
 * reason, if @iter was already being used, you should first cancel it
 * with g_variant_iter_cancel().
 *
 * After this call, @iter holds a reference to @value.  The reference
 * will be automatically dropped once all values have been iterated
 * over or manually by calling g_variant_iter_cancel().
 *
 * This function returns the number of times that
 * g_variant_iter_next() will return non-%NULL.  You're not expected to
 * use this value, but it's there incase you wanted to know.
 **/
gsize
g_variant_iter_init (GVariantIter *iter,
                     GVariant     *value)
{
  GVariantIterReal *real = (GVariantIterReal *) iter;

  g_assert (sizeof (GVariantIterReal) <= sizeof (GVariantIter));

  real->cancelled = FALSE;
  real->length = g_variant_n_children (value);
  real->offset = 0;
  real->child = NULL;

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
 * Retreives the next child value from @iter.  In the event that no
 * more child values exist, %NULL is returned and @iter drops its
 * reference to the value that it was created with.
 *
 * The return value of this function is internally cached by the
 * @iter, so you don't have to unref it when you're done.  For this
 * reason, thought, it is important to ensure that you call
 * g_variant_iter_next() one last time, even if you know the number of
 * items in the container.
 *
 * It is permissable to call this function on a cancelled iter, in
 * which case %NULL is returned and nothing else happens.
 **/
GVariant *
g_variant_iter_next (GVariantIter *iter)
{
  GVariantIterReal *real = (GVariantIterReal *) iter;

  if (real->child)
    {
      g_variant_unref (real->child);
      real->child = NULL;
    }

  if (real->value == NULL)
    return NULL;

  real->child = g_variant_get_child (real->value, real->offset++);

  if (real->offset == real->length)
    {
      g_variant_unref (real->value);
      real->value = NULL;
    }

  return real->child;
}

/**
 * g_variant_iter_cancel:
 * @iter: a #GVariantIter
 *
 * Causes @iter to drop its reference to the container that it was
 * created with.  You need to call this on an iter if, for some
 * reason, you stopped iterating before reading the end.
 *
 * You do not need to call this in the normal case of visiting all of
 * the elements.  In this case, the reference will be automatically
 * dropped by g_variant_iter_ntext() just before it returns %NULL.
 *
 * It is permissable to call this function more than once on the same
 * iter.  It is permissable to call this function after the last
 * value.
 **/
void
g_variant_iter_cancel (GVariantIter *iter)
{
  GVariantIterReal *real = (GVariantIterReal *) iter;

  real->cancelled = TRUE;

  if (real->value)
    {
      g_variant_unref (real->value);
      real->value = NULL;
    }

  if (real->child)
    {
      g_variant_unref (real->child);
      real->child = NULL;
    }
}

/**
 * g_variant_iter_was_cancelled:
 * @iter: a #GVariantIter
 * @returns: %TRUE if g_variant_iter_cancel() was called
 *
 * Determines if g_variant_iter_cancel() was called on @iter.
 **/
gboolean
g_variant_iter_was_cancelled (GVariantIter *iter)
{
  GVariantIterReal *real = (GVariantIterReal *) iter;

  return real->cancelled;
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

  return g_variant_load (G_VARIANT_TYPE_BOOLEAN,
                         &byte, 1, G_VARIANT_TRUSTED);
}

/**
 * g_variant_new_byte:
 * @byte: a #guint8 value
 * @returns: a new byte #GVariant instance
 *
 * Creates a new byte #GVariant instance.
 **/
GVariant *
g_variant_new_byte (guint8 byte)
{
  return g_variant_load (G_VARIANT_TYPE_BYTE,
                         &byte, 1, G_VARIANT_TRUSTED);
}

/**
 * g_variant_new_int16:
 * @int16: a #gint16 value
 * @returns: a new byte #GVariant instance
 *
 * Creates a new int16 #GVariant instance.
 **/
GVariant *
g_variant_new_int16 (gint16 int16)
{
  return g_variant_load (G_VARIANT_TYPE_INT16,
                         &int16, 2, G_VARIANT_TRUSTED);
}

/**
 * g_variant_new_uint16:
 * @uint16: a #guint16 value
 * @returns: a new byte #GVariant instance
 *
 * Creates a new uint16 #GVariant instance.
 **/
GVariant *
g_variant_new_uint16 (guint16 uint16)
{
  return g_variant_load (G_VARIANT_TYPE_UINT16,
                         &uint16, 2, G_VARIANT_TRUSTED);
}

/**
 * g_variant_new_int32:
 * @int32: a #gint32 value
 * @returns: a new byte #GVariant instance
 *
 * Creates a new int32 #GVariant instance.
 **/
GVariant *
g_variant_new_int32 (gint32 int32)
{
  return g_variant_load (G_VARIANT_TYPE_INT32,
                         &int32, 4, G_VARIANT_TRUSTED);
}

/**
 * g_variant_new_uint32:
 * @uint32: a #guint32 value
 * @returns: a new uint32 #GVariant instance
 *
 * Creates a new uint32 #GVariant instance.
 **/
GVariant *
g_variant_new_uint32 (guint32 uint32)
{
  return g_variant_load (G_VARIANT_TYPE_UINT32,
                         &uint32, 4, G_VARIANT_TRUSTED);
}

/**
 * g_variant_new_int64:
 * @int64: a #gint64 value
 * @returns: a new byte #GVariant instance
 *
 * Creates a new int64 #GVariant instance.
 **/
GVariant *
g_variant_new_int64 (gint64 int64)
{
  return g_variant_load (G_VARIANT_TYPE_INT64,
                         &int64, 8, G_VARIANT_TRUSTED);
}

/**
 * g_variant_new_uint64:
 * @uint64: a #guint64 value
 * @returns: a new uint64 #GVariant instance
 *
 * Creates a new uint64 #GVariant instance.
 **/
GVariant *
g_variant_new_uint64 (guint64 uint64)
{
  return g_variant_load (G_VARIANT_TYPE_UINT64,
                         &uint64, 8, G_VARIANT_TRUSTED);
}

/**
 * g_variant_new_double:
 * @floating: a #gdouble floating point value
 * @returns: a new double #GVariant instance
 *
 * Creates a new double #GVariant instance.
 **/
GVariant *
g_variant_new_double (gdouble floating)
{
  return g_variant_load (G_VARIANT_TYPE_DOUBLE,
                         &floating, 8, G_VARIANT_TRUSTED);
}

/**
 * g_variant_new_string:
 * @string: a normal C nul-terminated string
 * @returns: a new string #GVariant instance
 *
 * Creates a string #GVariant with the contents of @string.
 **/
GVariant *
g_variant_new_string (const gchar *string)
{
  return g_variant_load (G_VARIANT_TYPE_STRING,
                         string, strlen (string) + 1, G_VARIANT_TRUSTED);
}

/**
 * g_variant_new_object_path:
 * @string: a normal C nul-terminated string
 * @returns: a new object path #GVariant instance
 *
 * Creates a DBus object path #GVariant with the contents of @string.
 * @string must be a valid DBus object path.  Use
 * g_variant_is_object_path() if you're not sure.
 **/
GVariant *
g_variant_new_object_path (const gchar *string)
{
  g_assert (g_variant_is_object_path (string));

  return g_variant_load (G_VARIANT_TYPE_OBJECT_PATH,
                         string, strlen (string) + 1,
                         G_VARIANT_TRUSTED);
}

/**
 * g_variant_is_object_path:
 * @string: a normal C nul-terminated string
 * @returns: %TRUE if @string is a DBus object path
 *
 * Determines if a given string is a valid DBus object path.  You
 * should ensure that a string is a valid DBus object path before
 * passing it to g_variant_new_object_path().
 *
 * A valid object path starts with '/' followed by zero or more
 * sequences of characters separated by '/' characters.  Each sequence
 * must contain only the characters "[A-Z][a-z][0-9]_".  No sequence
 * (including the one following the final '/' character) may be empty.
 **/
gboolean
g_variant_is_object_path (const gchar *string)
{
  /* according to DBus specification */
  gsize i;

  /* The path must begin with an ASCII '/' (integer 47) character */
  if (string[0] != '/')
    return FALSE;

  for (i = 1; string[i]; i++)
    /* Each element must only contain the ASCII characters
     * "[A-Z][a-z][0-9]_" 
     */
    if (g_ascii_isalnum (string[i]) || string[i] == '_')
      ;

    /* must consist of elements separated by slash characters. */
    else if (string[i] == '/')
      {
        /* No element may be the empty string. */
        /* Multiple '/' characters cannot occur in sequence. */
        if (string[i - 1] == '/')
          return FALSE;
      }

    else
      return FALSE;

  /* A trailing '/' character is not allowed unless the path is the
   * root path (a single '/' character).
   */
  if (i > 1 && string[i - 1] == '/')
    return FALSE;

  return TRUE;
}

/**
 * g_variant_new_signature:
 * @string: a normal C nul-terminated string
 * @returns: a new signature #GVariant instance
 *
 * Creates a DBus type signature #GVariant with the contents of
 * @string.  @string must be a valid DBus type signature.  Use
 * g_variant_is_signature() if you're not sure.
 **/
GVariant *
g_variant_new_signature (const gchar *string)
{
  g_assert (g_variant_is_signature (string));

  return g_variant_load (G_VARIANT_TYPE_SIGNATURE,
                         string, strlen (string) + 1,
                         G_VARIANT_TRUSTED);
}

/**
 * g_variant_is_signature:
 * @string: a normal C nul-terminated string
 * @returns: %TRUE if @string is a DBus type signature
 *
 * Determines if a given string is a valid DBus type signature.  You
 * should ensure that a string is a valid DBus object path before
 * passing it to g_variant_new_signature().
 *
 * DBus type signatures consist of zero or more concrete #GVariantType
 * strings in sequence.
 **/
gboolean
g_variant_is_signature (const gchar *string)
{
  gsize first_invalid;

  /* make sure no non-concrete characters appear */
  first_invalid = strspn (string, "ybnqiuxtdvmasog(){}");
  if (string[first_invalid])
    return FALSE;

  /* make sure each type string is well-formed */
  while (*string)
    if (!g_variant_type_string_scan (&string, NULL))
      return FALSE;

  return TRUE;
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
                             g_variant_is_trusted (value));
}

/**
 * g_variant_get_boolean:
 * @value: a boolean #GVariant instance
 * @returns: %TRUE or %FALSE
 *
 * Returns the boolean value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_BOOLEAN.
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
 * other than %G_VARIANT_TYPE_BYTE.
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
 * g_variant_get_int16:
 * @value: a int16 #GVariant instance
 * @returns: a #gint16
 *
 * Returns the 16-bit signed integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_INT16.
 **/
gint16
g_variant_get_int16 (GVariant *value)
{
  gint16 int16;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_INT16));
  g_variant_store (value, &int16);

  return int16;
}

/**
 * g_variant_get_uint16:
 * @value: a uint16 #GVariant instance
 * @returns: a #guint16
 *
 * Returns the 16-bit unsigned integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_UINT16.
 **/
guint16
g_variant_get_uint16 (GVariant *value)
{
  guint16 uint16;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_UINT16));
  g_variant_store (value, &uint16);

  return uint16;
}

/**
 * g_variant_get_int32:
 * @value: a int32 #GVariant instance
 * @returns: a #gint32
 *
 * Returns the 32-bit signed integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_INT32.
 **/
gint32
g_variant_get_int32 (GVariant *value)
{
  gint32 int32;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_INT32));
  g_variant_store (value, &int32);

  return int32;
}

/**
 * g_variant_get_uint32:
 * @value: a uint32 #GVariant instance
 * @returns: a #guint32
 *
 * Returns the 32-bit unsigned integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_UINT32.
 **/
guint32
g_variant_get_uint32 (GVariant *value)
{
  guint32 uint32;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_UINT32));
  g_variant_store (value, &uint32);

  return uint32;
}

/**
 * g_variant_get_int64:
 * @value: a int64 #GVariant instance
 * @returns: a #gint64
 *
 * Returns the 64-bit signed integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_INT64.
 **/
gint64
g_variant_get_int64 (GVariant *value)
{
  gint64 int64;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_INT64));
  g_variant_store (value, &int64);

  return int64;
}

/**
 * g_variant_get_uint64:
 * @value: a uint64 #GVariant instance
 * @returns: a #guint64
 *
 * Returns the 64-bit unsigned integer value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_UINT64.
 **/
guint64
g_variant_get_uint64 (GVariant *value)
{
  guint64 uint64;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_UINT64));
  g_variant_store (value, &uint64);

  return uint64;
}

/**
 * g_variant_get_double:
 * @value: a double #GVariant instance
 * @returns: a #gdouble
 *
 * Returns the double precision floating point value of @value.
 *
 * It is an error to call this function with a @value of any type
 * other than %G_VARIANT_TYPE_DOUBLE.
 **/
gdouble
g_variant_get_double (GVariant *value)
{
  gdouble floating;

  g_assert (g_variant_matches (value, G_VARIANT_TYPE_DOUBLE));
  g_variant_store (value, &floating);

  return floating;
}

/**
 * g_variant_get_string:
 * @value: a string #GVariant instance
 * @length: a pointer to a #gsize, to store the length
 * @returns: the constant string
 *
 * Returns the string value of a #GVariant instance with a string
 * type.  This includes the types %G_VARIANT_TYPE_STRING,
 * %G_VARIANT_TYPE_OBJECT_PATH and %G_VARIANT_TYPE_SIGNATURE.
 *
 * If @length is non-%NULL then the length of the string (in bytes) is
 * returned there.  For trusted values, this information is already
 * known.  For untrusted values, a strlen() will be performed.
 *
 * It is an error to call this function with a @value of any type
 * other than those three.
 *
 * The return value remains valid as long as @value exists.
 **/
const gchar *
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

/**
 * g_variant_dup_string:
 * @value: a string #GVariant instance
 * @length: a pointer to a #gsize, to store the length
 * @returns: a newly allocated string
 *
 * Similar to g_variant_get_string() except that instead of returning
 * a constant string, the string is duplicated.
 *
 * The return value must be free'd using g_free().
 **/
gchar *
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

/**
 * g_variant_get_variant:
 * @value: a variant #GVariance instance
 * @returns: the item contained in the variant
 *
 * Unboxes @value.  The result is the #GVariant instance that was
 * contained in @value.
 **/
GVariant *
g_variant_get_variant (GVariant *value)
{
  g_assert (g_variant_matches (value, G_VARIANT_TYPE_VARIANT));

  return g_variant_get_child (value, 0);
}

/**
 * GVariantBuilder:
 *
 * An opaque type used to build container #GVariant instances one
 * child value at a time.
 */
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

/**
 * G_VARIANT_BUILDER_ERROR:
 *
 * Error domain for #GVariantBuilder. Errors in this domain will be
 * from the #GVariantBuilderError enumeration.  See #GError for
 * information on error domains. 
 **/
/**
 * GVariantBuilderError:
 * @G_VARIANT_BUILDER_ERROR_TOO_MANY: too many items have been added
 * (returned by g_variant_builder_check_add())
 * @G_VARIANT_BUILDER_ERROR_TOO_FEW: too few items have been added
 * (returned by g_variant_builder_check_end())
 * @G_VARIANT_BUILDER_ERROR_INFER: unable to infer the type of an
 * array or maybe (returned by g_variant_builder_check_end())
 * @G_VARIANT_BUILDER_ERROR_TYPE: the value is of the incorrect type
 * (returned by g_variant_builder_check_add())
 *
 * Errors codes returned by g_variant_builder_check_add() and
 * g_variant_builder_check_end().
 */

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

/**
 * g_variant_builder_add_value:
 * @builder: a #GVariantBuilder
 * @value: a #GVariant
 *
 * Adds @value to @builder.
 *
 * It is an error to call this function if @builder has an outstanding
 * child.  It is an error to call this function in any case that
 * g_variant_builder_check_add() would return %FALSE.
 **/
void
g_variant_builder_add_value (GVariantBuilder *builder,
                             GVariant        *value)
{
  GError *error = NULL;

  if G_UNLIKELY (!g_variant_builder_check_add_value (builder, value, &error))
    g_error ("g_variant_builder_add_value: %s", error->message);

  builder->trusted &= g_variant_is_trusted (value);

  if (builder->expected &&
      (builder->class == G_VARIANT_TYPE_CLASS_STRUCT ||
       builder->class == G_VARIANT_TYPE_CLASS_DICT_ENTRY))
    builder->expected = g_variant_type_next (builder->expected);

  if (builder->offset == builder->children_allocated)
    g_variant_builder_resize (builder, builder->children_allocated * 2);

  builder->children[builder->offset++] = g_variant_ref_sink (value);
}

/**
 * g_variant_builder_open:
 * @parent: a #GVariantBuilder
 * @class: a #GVariantTypeClass
 * @type: a #GVariantType, or %NULL
 * @returns: a new (child) #GVariantBuilder
 *
 * Opens a subcontainer inside the given @parent.
 *
 * It is not permissible to use any other builder calls with @parent
 * until @g_variant_builder_close() is called on the return value of
 * this function.
 *
 * It is an error to call this function if @parent has an outstanding
 * child.  It is an error to call this function in any case that
 * g_variant_builder_check_add() would return %FALSE.
 *
 * If @type is %NULL and @parent was given type information, that
 * information is passed down to the subcontainer and constrains what
 * values may be added to it.
 **/
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

/**
 * g_variant_builder_close:
 * @child: a #GVariantBuilder
 * @returns: the original parent of @child
 *
 * This function closes a builder that was created with a call to
 * g_variant_builder_open().
 *
 * It is an error to call this function on a builder that was created
 * using g_variant_builder_new().  It is an error to call this
 * function if @child has an outstanding child.  It is an error to
 * call this function in any case that g_variant_builder_check_end()
 * would return %FALSE.
 **/
GVariantBuilder *
g_variant_builder_close (GVariantBuilder *child)
{
  GVariantBuilder *parent;
  GVariant *value;

  g_assert (child->has_child == FALSE);
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

/**
 * g_variant_builder_new:
 * @class: a container #GVariantTypeClass
 * @type: a type contained in @class, or %NULL
 * @returns: a #GVariantBuilder
 *
 * Creates a new #GVariantBuilder.
 *
 * @class must be specified and must be a container type.
 *
 * If @type is given, it constrains the child values that it is
 * permissible to add.  If @class is not %G_VARIANT_TYPE_CLASS_VARIANT
 * then @type must be contained in @class and will be the @type of the
 * final value.  If @class is %G_VARIANT_TYPE_CLASS_VARIANT then @type
 * is the type of the value that must be added to the variant.
 *
 * After the builder is created, values are added using
 * g_variant_builder_add_value().
 *
 * After all the child values are added, g_variant_builder_end() ends
 * the process. 
 **/
GVariantBuilder *
g_variant_builder_new (GVariantTypeClass   class,
                       const GVariantType *type)
{
  GVariantBuilder *builder;

  g_assert (g_variant_type_class_is_container (class));
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

/**
 * g_variant_builder_end:
 * @builder: a #GVariantBuilder
 * @returns: a new, floating, #GVariant
 *
 * Ends the builder process and returns the constructed value.
 *
 * It is an error to call this function on a #GVariantBuilder created
 * by a call to g_variant_builder_open().  It is an error to call this
 * function if @builder has an outstanding child.  It is an error to
 * call this function in any case that g_variant_builder_check_end()
 * would return %FALSE.
 **/
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

/**
 * g_variant_builder_check_end:
 * @builder: a #GVariantBuilder
 * @error: a #GError
 * @returns: %TRUE if ending is safe
 *
 * Checks if a call to g_variant_builder_end() or
 * g_variant_builder_close() would succeed.
 *
 * It is an error to call this function if @builder has a child (ie:
 * g_variant_builder_open() has been used on @builder and
 * g_variant_builder_close() has not yet been called).
 *
 * This function checks that a sufficient number of items have been
 * added to the builder.  For dictionary entries, for example, it
 * ensures that 2 items were added.
 *
 * This function also checks that array and maybe builders that were
 * created without further type information contain at least one item
 * (without which it would be impossible to infer the type).
 *
 * If some sort of error (either too few items were added or type
 * inference is not possible) prevents the builder from being ended
 * then %FALSE is returned and @error is set.
 **/
gboolean
g_variant_builder_check_end (GVariantBuilder  *builder,
                             GError          **error)
{
  g_assert (builder != NULL);
  g_assert (builder->has_child == FALSE);

  switch (builder->class)
  {
    case G_VARIANT_TYPE_CLASS_VARIANT:
      if (builder->offset < 1)
        {
          g_set_error (error, G_VARIANT_BUILDER_ERROR,
                       G_VARIANT_BUILDER_ERROR_TOO_FEW,
                       "a variant must contain exactly one value");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_ARRAY:
      if (builder->type == NULL && builder->offset == 0)
        {
          g_set_error (error, G_VARIANT_BUILDER_ERROR,
                       G_VARIANT_BUILDER_ERROR_INFER,
                       "unable to infer type for empty array");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_MAYBE:
      if (builder->type == NULL && builder->offset == 0)
        {
          g_set_error (error, G_VARIANT_BUILDER_ERROR,
                       G_VARIANT_BUILDER_ERROR_INFER,
                       "unable to infer type for maybe with no value");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      if (builder->offset < 2)
        {
          g_set_error (error, G_VARIANT_BUILDER_ERROR,
                       G_VARIANT_BUILDER_ERROR_TOO_FEW,
                       "a dictionary entry must have a key and a value");
          return FALSE;
        }
      break;

    case G_VARIANT_TYPE_CLASS_STRUCT:
      if (builder->expected)
        {
          gchar *type_string;

          type_string = g_variant_type_dup_string (builder->type);
          g_set_error (error, G_VARIANT_BUILDER_ERROR,
                       G_VARIANT_BUILDER_ERROR_TOO_FEW,
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

/**
 * g_variant_builder_check_add:
 * @builder: a #GVariantBuilder
 * @class: a #GVariantTypeClass
 * @type: a #GVariantType, or %NULL
 * @error: a #GError
 * @returns: %TRUE if adding is safe
 *
 * Does all sorts of checks to ensure that it is safe to call
 * g_variant_builder_add() or g_variant_builder_open().
 *
 * It is an error to call this function if @builder has a child (ie:
 * g_variant_builder_open() has been used on @builder and
 * g_variant_builder_close() has not yet been called).
 *
 * It is an error to call this function with an invalid @class
 * (including %G_VARIANT_TYPE_CLASS_INVALID).
 *
 * If @type is non-%NULL this function first checks that it is a
 * member of @class (except, as with g_variant_builder_new(), if
 * @class is %G_VARIANT_TYPE_CLASS_VARIANT then any @type is OK).
 *
 * The function then checks if a child of class @class (and type
 * @type, if given) would be suitable for adding to the builder.
 * Errors are flagged in the event that the builder contains too many
 * items or the addition would cause a type error.
 *
 * If @class is specified and is a container type and @type is not
 * given then there is no guarantee that adding a value of that class
 * would not fail.  Calling g_variant_builder_open() with that @class
 * (and @type as %NULL) would succeed, though.
 *
 * In the case that any error is detected @error is set and %FALSE is
 * returned.
 **/
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

/**
 * g_variant_builder_cancel:
 * @builder: a #GVariantBuilder
 *
 * Cancels the build process.  All memory associated with @builder is
 * free'd.  If the builder was created with g_variant_builder_open()
 * then all ancestors are also free'd.
 **/
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

/**
 * g_variant_flatten:
 * @value: a #GVariant instance
 *
 * Flattens @value.
 *
 * This is a strange function with no direct effects but some
 * noteworthy side-effects.  Essentially, it ensures that @value is in
 * its most favourable form.  This involves ensuring that the value is
 * serialised and in machine byte order.  The investment of time now
 * can pay off by allowing shorter access times for future calls and
 * typically results in a reduction of memory consumption.
 *
 * A value received over the network or read from the disk in machine
 * byte order is already flattened.
 *
 * Some of the effects of this call are that any future accesses to
 * the data of @value (or children taken from it after flattening)
 * will occur in O(1) time.  Also, any data accessed from such a child
 * value will continue to be valid even after the child has been
 * destroyed, as long as @value still exists (since the contents of
 * the children are now serialised as part of the parent).
 **/
void
g_variant_flatten (GVariant *value)
{
  g_variant_get_data (value);
}

/**
 * g_variant_get_type_string:
 * @value: a #GVariant
 * @returns: the type string for the type of @value
 *
 * Returns the type string of @value.  Unlike the result of calling
 * g_variant_type_string_peek(), this string is nul-terminated.  This
 * string belongs to #GVariant and must not be free'd.
 **/
const gchar *
g_variant_get_type_string (GVariant *value)
{
  return (const gchar *) g_variant_get_type (value);
}

/**
 * g_variant_get_type_class:
 * @value: a #GVariant
 * @returns: the #GVariantTypeClass of @value
 *
 * Returns the type class of @value.  This function is equivalent to
 * calling g_variant_get_type() followed by
 * g_variant_type_get_class().
 **/
GVariantTypeClass
g_variant_get_type_class (GVariant *value)
{
  return g_variant_type_get_class (g_variant_get_type (value));
}

/**
 * g_variant_is_basic:
 * @value: a #GVariant
 * @returns: %TRUE if @value has a basic type
 *
 * Determines if @value has a basic type.  Values with basic types may
 * be used as the keys of dictionary entries.
 *
 * This function is the exact opposite of g_variant_is_container().
 **/
gboolean
g_variant_is_basic (GVariant *value)
{
  return g_variant_type_class_is_basic (g_variant_get_type_class (value));
}

/**
 * g_variant_is_container:
 * @value: a #GVariant
 * @returns: %TRUE if @value has a basic type
 *
 * Determines if @value has a container type.  Values with container
 * types may be used with the functions g_variant_n_children() and
 * g_variant_get_child().
 *
 * This function is the exact opposite of g_variant_is_basic().
 **/
gboolean
g_variant_is_container (GVariant *value)
{
  return g_variant_type_class_is_container (g_variant_get_type_class (value));
}

#include <glib/gstdio.h>
void
g_variant_dump_data (GVariant *value)
{
  const guchar *data;
  const guchar *end;
  char row[3*16+2];
  gsize data_size;
  gsize i;

  data_size = g_variant_get_size (value);

  g_debug ("GVariant at %p (type '%s', %d bytes):",
           value, g_variant_get_type_string (value), data_size);

  data = g_variant_get_data (value);
  end = data + data_size;

  i = 0;
  row[3*16+1] = '\0';
  while (data < end || (i & 15))
    {
      if ((i & 15) == (((gsize) data) & 15) && data < end)
        g_sprintf (&row[3 * (i & 15) + (i & 8)/8], "%02x  ", *data++);
      else
        g_sprintf (&row[3 * (i & 15) + (i & 8)/8], "    ");

      if ((++i & 15) == 0)
        {
          g_debug ("   %s", row);
          memset (row, 'q', 3 * 16 + 1);
        }
    }

  g_debug ("==");
}
