/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#include "gvarianttype.h"

#include <glib/gtestutils.h>
#include "gexpensive.h"

#include <string.h>

static gboolean
g_variant_type_check (const GVariantType *type)
{
  const gchar *type_string = (const gchar *) type;

  return g_variant_type_string_scan (&type_string, NULL);
}

/**
 * g_variant_type_string_scan:
 * @type_string: a pointer to any string
 * @limit: the end of @string, or %NULL
 * @returns: %TRUE if a valid type string was found
 *
 * Scan for a single complete and valid #GVariantType type string in
 * @type_string.  The memory pointed to by @limit (or bytes beyond it)
 * is never accessed.
 *
 * If a valid type string is found, @type_string is updated to point
 * to the first character past the end of the string that was found
 * and %TRUE is returned.
 *
 * If there is no valid type string starting at @type_string, or if
 * the type string does not end before @limit then %FALSE is returned
 * and the state of @type_string is undefined.
 *
 * For the simple case of checking if a string is a valid type string,
 * see g_variant_type_string_is_valid().
 **/
gboolean
g_variant_type_string_scan (const gchar **type_string,
                            const gchar  *limit)
{
  if (*type_string == limit)
    return FALSE;

  switch (*(*type_string)++)
  {
    case '\0':
      return FALSE;

    case '(':
      while (*type_string != limit && **type_string != ')')
        if (!g_variant_type_string_scan (type_string, limit))
          return FALSE;

      if (*type_string == limit)
        return FALSE;

      (*type_string)++; /* ')' */

      return TRUE;

    case '{':
      if (*type_string == limit || **type_string == '\0')
        return FALSE;

      if (!strchr ("bynqiuxtdsog", *(*type_string)++))         /* key */
        return FALSE;

      if (!g_variant_type_string_scan (type_string, limit))    /* value */
        return FALSE;

      if (*type_string == limit || *(*type_string)++ != '}')
        return FALSE;

      return TRUE;

    case 'm': case 'a':
      return g_variant_type_string_scan (type_string, limit);  /* tailcall */

    case 'b': case 'y': case 'n': case 'q': case 'i': case 'u':
    case 'x': case 't': case 'd': case 's': case 'o': case 'g':
    case 'v': case 'r': case 'e': case '*':
      return TRUE;

    default:
      return FALSE;
  }
}

/**
 * g_variant_type_string_is_valid:
 * @type_string: a pointer to any string
 * @returns: %TRUE if @type_string is exactly one valid type string
 *
 * Checks if @type_string is a valid #GVariantType type string.  This
 * call is equivalent to calling g_variant_type_string_scan() and
 * confirming that the following character is a nul terminator.
 **/
gboolean
g_variant_type_string_is_valid (const gchar *type_string)
{
  if (!g_variant_type_string_scan (&type_string, NULL))
    return FALSE;

  if (*type_string != '\0')
    return FALSE;

  return TRUE;
}

/**
 * g_variant_type_free:
 * @type: a #GVariantType
 *
 * Frees a #GVariantType that was allocated with
 * g_variant_type_copy(), g_variant_type_new() or one of the container
 * type constructor functions.
 **/
void
g_variant_type_free (GVariantType *type)
{
  g_free (type);
}

/**
 * g_variant_type_copy:
 * @type: a #GVariantType
 *
 * Makes a copy of a #GVariantType.  This copy must be freed using
 * g_variant_type_free().
 **/
GVariantType *
g_variant_type_copy (const GVariantType *type)
{
  return g_memdup (type, g_variant_type_get_string_length (type));
}

/**
 * g_variant_type_new:
 * @type_string: a valid #GVariantType type string
 * @returns: a new #GVariantType
 *
 * Creates a new #GVariantType corresponding to the type string given
 * by @type_string.  This new type must be freed using
 * g_variant_type_free().
 */
GVariantType *
g_variant_type_new (const gchar *type_string)
{
  return g_variant_type_copy (G_VARIANT_TYPE (type_string));
}

/**
 * g_variant_type_get_string_length:
 * @type: a #GVariantType
 * @returns: the length of the corresponding type string
 *
 * Returns the length of the type string corresponding to the given
 * @type.  This function must be used to determine the valid extent of
 * the memory region returned by g_variant_type_peek_string().
 **/
gsize
g_variant_type_get_string_length (const GVariantType *type)
{
  const gchar *type_string = (const gchar *) type;
  gsize index;
  gint brackets;

  brackets = 0;

  do
    {
      while (type_string[index] == 'a' || type_string[index] == 'm')
        index++;

      if (type_string[index]== '(' || type_string[index] == '{')
        brackets++;

      else if (type_string[index] == ')' || type_string[index] == '}')
        brackets--;

      index++;
    }
  while (brackets);

  return index;
}

/**
 * g_variant_type_peek_string:
 * @type: a #GVariantType
 * @returns: the corresponding type string (non-terminated)
 *
 * Returns the type string corresponding to the given @type.  The
 * result is not nul-terminated; in order to determine its length you
 * must call g_variant_type_get_string_length().
 *
 * To get a nul-terminated string, see g_variant_type_dup_string().
 **/
const gchar *
g_variant_type_peek_string (const GVariantType *type)
{
  return (const gchar *) type;
}

/**
 * g_variant_type_dup_string:
 * @type: a #GVariantType
 * @returns: the corresponding type string (must be freed)
 *
 * Returns a newly-allocated copy of the type string corresponding to
 * @type.  The return result must be freed using g_free().
 **/
gchar *
g_variant_type_dup_string (const GVariantType *type)
{
  return g_strndup (g_variant_type_peek_string (type),
                    g_variant_type_get_string_length (type));
}

const GVariantType *
_g_variant_type_check_string (const gchar *type_string)
{
  g_assert (g_variant_type_string_is_valid (type_string));

  return (GVariantType *) type_string;
}

/**
 * g_variant_type_is_concrete:
 * @type: a #GVariantType
 * @returns: %TRUE if @type is concrete
 *
 * Determines if the given @type is a concrete (ie: non-wildcard)
 * type.  A #GVariant instance may only have a concrete type.
 *
 * A type is concrete if its type string does not contain any wildcard
 * characters ('*', 'r' or 'e').
 **/
gboolean
g_variant_type_is_concrete (const GVariantType *type)
{
  const gchar *type_string = g_variant_type_peek_string (type);
  gsize type_length = g_variant_type_get_string_length (type);
  gsize i;

  for (i = 0; i < type_length; i++)
    if (strchr ("*?@&re", type_string[i]))
      return FALSE;

  return TRUE;
}

/**
 * g_variant_type_is_container:
 * @type: a #GVariantType
 * @returns: %TRUE if @type is a container type
 *
 * Determines if the given @type is a container type.
 *
 * Container types are any array, maybe, structure, or dictionary
 * entry types plus the variant type.
 *
 * This function returns %TRUE for any wildcard type for which every
 * matching concrete type is a container.  This does not include
 * %G_VARIANT_TYPE_ANY.
 **/
gboolean
g_variant_type_is_container (const GVariantType *type)
{
  gchar first_char = g_variant_type_peek_string (type)[0];

  switch (first_char)
  {
    case 'a':
    case 'm':
    case 'r':
    case '(':
    case 'e':
    case '{':
    case 'v':
      return TRUE;

    default:
      return FALSE;
  }
}

/**
 * g_variant_type_is_basic:
 * @type: a #GVariantType
 * @returns: %TRUE if @type is a basic type
 *
 * Determines if the given @type is a basic type.
 *
 * Basic types are booleans, bytes, integers, doubles, strings, object
 * paths and signatures.
 *
 * Only a basic type may be used as the key of a dictionary entry.
 *
 * This function returns %FALSE for all wildcard types.
 **/
gboolean
g_variant_type_is_basic (const GVariantType *type)
{
  gchar first_char = g_variant_type_peek_string (type)[0];

  switch (first_char)
  {
    case 'b':
    case 'y':
    case 'n':
    case 'q':
    case 'i':
    case 'u':
    case 't':
    case 'x':
    case 's':
    case 'o':
    case 'g':
    case '?':
    case '&':
      return TRUE;

    default:
      return FALSE;
  }
}

/**
 * g_variant_type_is_fixed_size:
 * @type: a #GVariantType
 * @returns: %TRUE if @type is a fixed-size type
 *
 * Determines if the given type is a fixed-size type.
 *
 * Fixed sized types are booleans, bytes, integers, doubles and
 * structures and dictionary entries made out of fixed types.
 **/
gboolean
g_variant_type_is_fixed_size (const GVariantType *type)
{
  const gchar *type_string = g_variant_type_peek_string (type);
  gsize length = g_variant_type_get_string_length (type);
  gsize i;

  for (i = 0; i < length; i++)
   if (strchr ("am*?sog", type_string[i]))
     return FALSE;

  return TRUE;
}

/**
 * g_variant_type_hash:
 * @type: a #GVariantType
 * @returns: the hash value
 *
 * Hashes @type.
 *
 * The argument type of @type is only #gconstpointer to allow use with
 * #GHashTable without function pointer casting.  A valid
 * #GVariantType must be provided.
 **/
guint
g_variant_type_hash (gconstpointer type)
{
  const gchar *type_string = g_variant_type_peek_string (type);
  gsize length = g_variant_type_get_string_length (type);
  guint value = 0;
  gsize i;

  for (i = 0; i < length; i++)
    value = (value << 5) - value + type_string[i];

  return value;
}

/**
 * g_variant_type_equal:
 * @type1: a #GVariantType
 * @type2: a #GVariantType
 * @returns: %TRUE if @type1 and @type2 are exactly equal
 *
 * Compares @type1 and @type2 for equality.
 *
 * Only returns %TRUE if the types are exactly equal.  Even if one
 * type is a wildcard type and the other matches it, false will be
 * returned if they are not exactly equal.  If you want to check for
 * matching, use g_variant_type_matches().
 *
 * The argument types of @type1 and @type2 are only #gconstpointer to
 * allow use with #GHashTable without function pointer casting.  For
 * both arguments, a valid #GVariantType must be provided.
 **/
gboolean
g_variant_type_equal (gconstpointer type1,
                      gconstpointer type2)
{
  const gchar *string1, *string2;
  gsize size1, size2;

  if (type1 == type2)
    return TRUE;

  size1 = g_variant_type_get_string_length (type1);
  size2 = g_variant_type_get_string_length (type2);

  if (size1 != size2)
    return FALSE;

  string1 = g_variant_type_peek_string (type1);
  string2 = g_variant_type_peek_string (type2);

  return memcmp (string1, string2, size1) == 0;
}

/**
 * g_variant_type_matches:
 * @type: a #GVariantType
 * @pattern: a #GVariantType
 * @returns: %TRUE if @type matches @pattern
 *
 * Performs a pattern match between @type and @pattern.
 *
 * This function returns %TRUE if @type can be reached by making
 * @pattern less general (ie: by replacing zero or more wildcard
 * characters in the type string of @pattern with matching type
 * strings that possibly contain wildcards themselves).
 *
 * This function defines a bounded join-semilattice over #GVariantType
 * for which %G_VARIANT_TYPE_ANY is top.
 **/
gboolean
g_variant_type_matches (const GVariantType *type,
                        const GVariantType *pattern)
{
  const gchar *pattern_string;
  const gchar *pattern_end;
  const gchar *type_string;

  pattern_string = g_variant_type_peek_string (pattern);
  type_string = g_variant_type_peek_string (type);

  pattern_end = pattern_string + g_variant_type_get_string_length (pattern);

  /* we know that type and pattern are both well-formed, so it's
   * safe to treat this merely as a text processing problem.
   */
  while (pattern_string < pattern_end)
    {
      char pattern_char = *pattern_string++;

      if (pattern_char == *type_string)
        type_string++;

      else if (*type_string == ')')
        return FALSE;

      else
        {
          const GVariantType *target_type = G_VARIANT_TYPE (type_string);

          if (!g_variant_type_is_of_class (target_type, pattern_char))
            return FALSE;

          type_string += g_variant_type_get_string_length (target_type);
        }
    }

  return TRUE;
}

gboolean
g_variant_type_is_of_class (const GVariantType *type,
                            GVariantTypeClass   class)
{
  char first_char = *(const gchar *) type;

  switch (class)
  {
    case G_VARIANT_TYPE_CLASS_ALL:
      return TRUE;

    case G_VARIANT_TYPE_CLASS_FIXED:
      return g_variant_type_is_fixed_size (type);
      
    case G_VARIANT_TYPE_CLASS_BASIC:
      return g_variant_type_is_basic (type);

    case G_VARIANT_TYPE_CLASS_FIXED_BASIC:
      return g_variant_type_is_basic (type) &&
             g_variant_type_is_fixed_size (type);

    case G_VARIANT_TYPE_CLASS_STRUCT:
      return first_char == '(';

    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      return first_char == '{';

    default:
      return class == first_char;
  }
}

GVariantTypeClass
g_variant_type_get_natural_class (const GVariantType *type)
{
  char first_char = *(const gchar *) type;

  switch (first_char)
  {
    case '(':
      return G_VARIANT_TYPE_CLASS_STRUCT;

    case '{':
      return G_VARIANT_TYPE_CLASS_DICT_ENTRY;

    default:
      return first_char;
  }
}

gboolean
g_variant_type_class_is_container (GVariantTypeClass class)
{
  switch (class)
  {
    case G_VARIANT_TYPE_CLASS_VARIANT:
    case G_VARIANT_TYPE_CLASS_MAYBE:
    case G_VARIANT_TYPE_CLASS_ARRAY:
    case G_VARIANT_TYPE_CLASS_STRUCT:
    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      return TRUE;

    default:
      return FALSE;
  }
}

gboolean
g_variant_type_class_is_basic (GVariantTypeClass class)
{
  switch (class)
  {
    case G_VARIANT_TYPE_CLASS_BOOLEAN:
    case G_VARIANT_TYPE_CLASS_BYTE:
    case G_VARIANT_TYPE_CLASS_INT16:
    case G_VARIANT_TYPE_CLASS_UINT16:
    case G_VARIANT_TYPE_CLASS_INT32:
    case G_VARIANT_TYPE_CLASS_UINT32:
    case G_VARIANT_TYPE_CLASS_INT64:
    case G_VARIANT_TYPE_CLASS_UINT64:
    case G_VARIANT_TYPE_CLASS_DOUBLE:
    case G_VARIANT_TYPE_CLASS_STRING:
    case G_VARIANT_TYPE_CLASS_OBJECT_PATH:
    case G_VARIANT_TYPE_CLASS_SIGNATURE:
    case G_VARIANT_TYPE_CLASS_BASIC:
    case G_VARIANT_TYPE_CLASS_FIXED_BASIC:
      return TRUE;

    default:
      return FALSE;
  }
}

const GVariantType *
g_variant_type_element (const GVariantType *type)
{
  const gchar *type_string = g_variant_type_peek_string (type);

  g_assert (type_string[0] == 'a' || type_string[0] == 'm');

  return (const GVariantType *) &type_string[1];
}

const GVariantType *
g_variant_type_first (const GVariantType *type)
{
  const gchar *type_string = g_variant_type_peek_string (type);

  g_assert (type_string[0] == '(' || type_string[0] == '{');

  if (type_string[1] == ')')
    return NULL;

  return (const GVariantType *) &type_string[1];
}

const GVariantType *
g_variant_type_next (const GVariantType *type)
{
  const gchar *type_string = g_variant_type_peek_string (type);
  
  type_string += g_variant_type_get_string_length (type);

  if (*type_string == ')')
    return NULL;

  return (const GVariantType *) type_string;
}

gsize
g_variant_type_n_items (const GVariantType *type)
{
  gsize count = 0;

  type = g_variant_type_first (type);
  do
    count++;
  while ((type = g_variant_type_next (type)));

  return count;
}

const GVariantType *
g_variant_type_key (const GVariantType *type)
{
  const gchar *type_string = g_variant_type_peek_string (type);

  g_assert (type_string[0] == '{');

  return (const GVariantType *) &type_string[1];
}

const GVariantType *
g_variant_type_value (const GVariantType *type)
{
  return g_variant_type_next (g_variant_type_key (type));
}

GVariantType *
g_variant_type_new_struct (const gpointer     *items,
                           gsize               length,
                           GVariantTypeGetter  func)
{
  char buffer[1024];
  gsize offset;
  gsize i;

  G_BEGIN_EXPENSIVE_CHECKS {
    for (i = 0; i < length; i++)
      if (func)
        g_assert (g_variant_type_check (func (items[i])));
      else
        g_assert (g_variant_type_check (items[i]));
  } G_END_EXPENSIVE_CHECKS

  offset = 0;
  buffer[offset++] = '(';

  for (i = 0; i < length; i++)
    {
      const GVariantType *type;
      gsize size;

      if (func)
        type = func (items[i]);
      else
        type = items[i];

      size = g_variant_type_get_string_length (type);

      if (offset + size >= sizeof buffer) /* leave room for ')' */
        g_error ("You just requested creation of an extremely complex "
                 "structure type.  If you really want to do this, file "
                 "a bug to have this limitation lifted.");

      memcpy (&buffer[offset], type, size);
      offset += size;
    }

  g_assert (offset < sizeof buffer);
  buffer[offset++] = ')';

  return g_memdup (buffer, offset);
}

GVariantType *
g_variant_type_new_array (const GVariantType *element)
{
  gsize size;
  gchar *new;

  G_BEGIN_EXPENSIVE_CHECKS {
    g_assert (g_variant_type_check (element));
  } G_END_EXPENSIVE_CHECKS

  size = g_variant_type_get_string_length (element);
  new = g_malloc (size + 1);

  new[0] = 'a';
  memcpy (new + 1, element, size);

  return (GVariantType *) new;
}

GVariantType *
g_variant_type_new_maybe (const GVariantType *element)
{
  gsize size;
  gchar *new;

  G_BEGIN_EXPENSIVE_CHECKS {
    g_assert (g_variant_type_check (element));
  } G_END_EXPENSIVE_CHECKS

  size = g_variant_type_get_string_length (element);
  new = g_malloc (size + 1);

  new[0] = 'm';
  memcpy (new + 1, element, size);

  return (GVariantType *) new;
}

GVariantType *
g_variant_type_new_dict_entry (const GVariantType *key,
                               const GVariantType *value)
{
  gsize keysize, valsize;
  gchar *new;

  G_BEGIN_EXPENSIVE_CHECKS {
    g_assert (g_variant_type_check (key));
    g_assert (g_variant_type_check (value));
    g_assert (g_variant_type_is_basic (key));
  } G_END_EXPENSIVE_CHECKS

  keysize = g_variant_type_get_string_length (key);
  valsize = g_variant_type_get_string_length (value);

  new = g_malloc (1 + keysize + valsize + 1);

  new[0] = '{';
  memcpy (new + 1, key, keysize);
  memcpy (new + 1 + keysize, value, valsize);
  new[1 + keysize + valsize] = '}';

  return (GVariantType *) new;
}
