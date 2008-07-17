#include <unistd.h>
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

#include "gsignature-iterate.h"

#include "gsignature.h"

/**
 * SECTION: gsignature
 * @title: GSignature and GSignatureType
 * @short_description: the type signature system for #GVariant
 * @see_also: #GVariant
 *
 * This section describes the type signature system used by #GVariant.
 * Each #GVariant instance has a #GSignature (and corresponding
 * #GSignatureType).
 **/

/**
 * GSignature:
 *
 * #GSignature is an opaque datatype representing either a concrete
 * datatype or (through the use of wildcard positions) a range of
 * possible datatypes.
 *
 *
 * A concrete #GSignature is the signature of at least one #GVariant.  A
 * concrete signature has zero wildcard positions.
 *
 * An abstract #GSignature is one which matches a number of other
 * signatures through the use of wildcard positions.  See
 * g_signature_match() for a detailed description.
 *
 * #GSignature instances have a one-to-one mapping with "signature
 * strings".  A #GSignature can be made from a signature string with
 * g_signature_new() and a signature string can be obtained from a
 * #GSignature with g_signature_get().
 *
 * The set of signature strings is generated by the following grammer.
 * Terminals appear single-quoted and + is concatenation.
 *
 * <programlisting>
 *
 * signature      = base-signature |
 *                  'm' + signature |
 *                  'a' + signature |
 *                  '(' + signatures + ')' |
 *                  '{' + base-signature + signature + '}' |
 *                  'v'
 *
 * signatures     = signature |
 *                  signature + signatures
 *
 * base-signature = 'b' | 'y' | 'n' | 'q' | 'i' | 'u' | 'x' |
 *                  't' | 'd' | 's' | 'o' | 'g' | '*'
 *
 * </programlisting>
 *
 * The set of signature strings is a prefix code -- that is, no valid
 * signature is a prefix of another valid signature.
 *
 * Signature strings starting with 'm' correspond to maybe instances
 * which are either 'Nothing' or 'Just x' where x is an instance of
 * the signature following the 'm'
 *
 * Signature strings starting with 'a' correspond to arrays in which
 * each element has the signature following the 'a'.
 *
 * Signature strings starting with '(' correspond to structures
 * (tuples) of a number of child items (with signatures of the child
 * items concatenated between the brackets).
 *
 * Signature strings starting with '{' correspond to dictionary
 * entries.  The first signature inside the brackets is the signature
 * of the key (which must be a base signature) and the second
 * signature inside the brackets is the signature of the value.
 *
 * By convention, "a dictionary" is an array of dictionary entries.
 *
 * There is no trouble in having a '*' in the key position of a
 * dictionary entry signature string.  Signature matching is only
 * defined over valid signatures and no valid dictionary entry
 * signature has a non-base key signature.
 *
 * Any signature string containing a '*' forms an abstract #GSignature
 * with a 'wildcard position' at the location of the '*'.
 *
 * The meanings of the characters 'v', 'b', 'y', 'n', 'q', 'i', 'u',
 * 'x', 't', 'd', 's', 'o' and 'g' are given below.
 *
 * Internally, a #GSignature is essentially a pointer to a string of
 * characters.  It has some key differences from a normal C string,
 * however.
 *
 * First, a #GSignature may only point to a valid signature string
 * (whereas a C string can be any character data).
 *
 * Second, because signature strings are a prefix code,
 * nul-termination is not required -- the bytes pointed to by a
 * #GSignature may be followed by arbitrary junk.  This property is
 * used to allow extracting the signatures of structure members from
 * the structure signature without copying.
 *
 * You should not use this "internal knowledge" while programming, but
 * it may cause the memory allocation model to make a bit more sense.
 **/

/* g_signature_copy of a single-byte signature will
 * "allocate" the byte from this table.  this prevents
 * having to malloc in the most common cases.
 */
static const guint8 g_signature_some_bytes[] =
{
  [G_SIGNATURE_TYPE_BOOLEAN]        = G_SIGNATURE_TYPE_BOOLEAN,
  [G_SIGNATURE_TYPE_BYTE]           = G_SIGNATURE_TYPE_BYTE,
  [G_SIGNATURE_TYPE_INT16]          = G_SIGNATURE_TYPE_INT16,
  [G_SIGNATURE_TYPE_UINT16]         = G_SIGNATURE_TYPE_UINT16,
  [G_SIGNATURE_TYPE_INT32]          = G_SIGNATURE_TYPE_INT32,
  [G_SIGNATURE_TYPE_UINT32]         = G_SIGNATURE_TYPE_UINT32,
  [G_SIGNATURE_TYPE_INT64]          = G_SIGNATURE_TYPE_INT64,
  [G_SIGNATURE_TYPE_UINT64]         = G_SIGNATURE_TYPE_UINT64,
  [G_SIGNATURE_TYPE_DOUBLE]         = G_SIGNATURE_TYPE_DOUBLE,
  [G_SIGNATURE_TYPE_STRING]         = G_SIGNATURE_TYPE_STRING,
  [G_SIGNATURE_TYPE_OBJECT_PATH]    = G_SIGNATURE_TYPE_OBJECT_PATH,
  [G_SIGNATURE_TYPE_SIGNATURE]      = G_SIGNATURE_TYPE_SIGNATURE,
  [G_SIGNATURE_TYPE_VARIANT]        = G_SIGNATURE_TYPE_VARIANT,
  ['~']                             = '\0'
};

/**
 * g_signature_free:
 * @signature: a #GSignature
 *
 * Frees the memory associated with a #GSignature.
 *
 * This function should only be called on
 * signatures created with g_signature_copy(),
 * g_signature_new(), and the
 * container signature construction functions.
 *
 * Do not call this on signatures returned by
 * g_signature() or the container deconstruction
 * functions.
 **/
void
g_signature_free (GSignature signature)
{
  const guint8 *str = (const guint8 *) signature;

  if (str < &g_signature_some_bytes[sizeof (g_signature_some_bytes)] &&
      str >= &g_signature_some_bytes[0])
    return;

  g_free (signature);
}

/**
 * g_signature_copy:
 * @signature: a #GSignature
 * @returns: the copy #GSignature
 *
 * Makes a copy of @signature and returns it.  The
 * copy must be freed with g_signature_free().
 **/
GSignature
g_signature_copy (GSignature signature)
{
  const guint8 *str = (const guint8 *) signature;

  if (*str < sizeof (g_signature_some_bytes) &&
      g_signature_some_bytes[*str] != '\0')
    return (GSignature) &g_signature_some_bytes[*str];

  return (GSignature) g_strndup (g_signature_peek (signature),
                                 g_signature_length (signature));
}

/**
 * g_signature_new:
 * @string: a signature string
 * @returns: a new #GSignature
 *
 * Creates a new #GSignature from a signature
 * string.  The new signature must be freed with
 * g_signature_free().
 *
 * This is exactly equivalent to calling
 * g_signature() then g_signature_copy().  It is
 * an error to call this function with an invalid
 * signature string.
 **/
GSignature
g_signature_new (const char *string)
{
  return g_signature_copy (g_signature (string));
}

/**
 * g_signature:
 * @string: a signature string
 * @returns: a #GSignature
 *
 * Converts a string to the equivalent
 * #GSignature.  The returned signature must -not-
 * be freed with g_signature_free().  It remains
 * valid so long as the string remains valid and
 * unmodified.
 *
 * Is is an error to call this function with an
 * invalid signature string.
 **/
#undef g_signature
GSignature
g_signature (const char *string)
{
  g_assert (g_signature_is_valid (string));

  return (GSignature) string;
}

static const char *
g_signature_iterate (const char *signature_str)
{
  g_signature_iterate_start (signature_str) {
  } g_signature_iterate_return (signature_str);
}

/**
 * g_signature_length:
 * @signature: a #GSignature
 * @returns: the length of the signature
 *
 * Determines the length of the signature.  This
 * is equal to the length of the string that would
 * be returned by g_signature_get().
 *
 * This function should be used with
 * g_signature_peek() to determine how many bytes
 * of the return value are valid.
 **/
int
g_signature_length (GSignature signature)
{
  const char *str = (const char *) signature;
  const char *end;

  end = g_signature_iterate (str);

  return end - str;
}

/**
 * g_signature_peek:
 * @signature: a #GSignature
 * @returns: a pointer to the start of the
 * signature string
 *
 * Peeks at the internal contents of the
 * signature.  A pointer to the first character of
 * the signature is returned.  The return value is
 * not nul-terminated and its length must be
 * determined with g_signature_length().
 **/
const char *
g_signature_peek (GSignature signature)
{
  return (const char *) signature;
}

/**
 * g_signature_get:
 * @signature: a #GSignature
 * @returns: the signature, as a string
 *
 * Obtains the string format of @signature.
 *
 * The result consists of the first
 * g_signature_length() bytes of the data starting
 * at the result of g_signature_peek().
 *
 * The return value just be freed with g_free().
 **/
char *
g_signature_get (GSignature signature)
{
  return g_strndup (g_signature_peek (signature),
                    g_signature_length (signature));
}

static gboolean
g_signature_check (const char **signature)
{
  switch (*((*signature)++))
  {
    case '(':
      while (**signature != ')')
        if (g_signature_check (signature) == FALSE)
          return FALSE;

      if (*((*signature)++) != ')')
        return FALSE;

      return TRUE;

    case '{':
      if (g_signature_check (signature) == FALSE)
        return FALSE;
      if (g_signature_check (signature) == FALSE)
        return FALSE;

      if (*((*signature)++) != '}')
        return FALSE;

      return TRUE;

    case 'm':
    case 'a':
      return g_signature_check (signature);

    case G_SIGNATURE_TYPE_BOOLEAN:
    case G_SIGNATURE_TYPE_BYTE:
    case G_SIGNATURE_TYPE_INT16:
    case G_SIGNATURE_TYPE_UINT16:
    case G_SIGNATURE_TYPE_INT32:
    case G_SIGNATURE_TYPE_UINT32:
    case G_SIGNATURE_TYPE_INT64:
    case G_SIGNATURE_TYPE_UINT64:
    case G_SIGNATURE_TYPE_DOUBLE:
    case G_SIGNATURE_TYPE_STRING:
    case G_SIGNATURE_TYPE_OBJECT_PATH:
    case G_SIGNATURE_TYPE_SIGNATURE:
    case G_SIGNATURE_TYPE_VARIANT:
    case G_SIGNATURE_TYPE_ANY:
      return TRUE;

    default:
      return FALSE;
  }
}

/**
 * g_signature_is_valid:
 * @string: a possible signature string
 * @returns: %TRUE if the string is valid
 *
 * Determines if a string is a valid signature
 * string.  See the overview for what makes a
 * valid signature string.
 **/
gboolean
g_signature_is_valid (const char *string)
{
  const char *end;

  end = string;

  if (!g_signature_check (&end))
    return FALSE;

  if (*end != '\0')
    return FALSE;

  return TRUE;
}

/**
 * g_signature_has_type:
 * @signature: a #GSignature
 * @type: a #GSignatureType
 * @returns: %TRUE if @signature has @type
 *
 * Determines if @signature is of the type @type.
 * This is equivalent to checking the return value
 * of g_signature_type() for equality with @type.
 **/
gboolean
g_signature_has_type (GSignature     signature,
                      GSignatureType type)
{
  switch (type)
  {
    case G_SIGNATURE_TYPE_STRUCT:
      return '(' == *(const char *) signature;

    case G_SIGNATURE_TYPE_DICT_ENTRY:
      return '{' == *(const char *) signature;

    default:
      return type == *(const char *) signature;
  }
}

/**
 * g_signature_type:
 * @signature: a #GSignature
 * @returns: the #GSignatureType of @signature
 *
 * Determines the type of @signature (e.g. array,
 * structure, string, etc).
 **/
GSignatureType
g_signature_type (GSignature signature)
{
  switch (*(const char *) signature)
  {
    case '(':
      return G_SIGNATURE_TYPE_STRUCT;

    case '{':
      return G_SIGNATURE_TYPE_DICT_ENTRY;

    default:
      return *(const char *) signature;
  }
}

/**
 * g_signature_concrete:
 * @signature: a #GSignature
 * @returns: %TRUE if @signature is concrete
 *
 * Determines if a signature represents a concrete
 * type.  A signature is concrete if it contains
 * no wildcards.
 **/
gboolean
g_signature_concrete (GSignature signature)
{
  g_signature_iterate_start (signature)
  {
    case '*':
      return FALSE;
  }
  g_signature_iterate_end;

  return TRUE;
}

/**
 * g_signature_hash:
 * @signature: a #GSignature
 * @returns: the hash value
 *
 * Computes the deterministic hash value for a
 * signature.  This function is suitable for use
 * with #GHashTable.
 **/
guint
g_signature_hash (gconstpointer signature)
{
  guint value = 0;

  g_signature_iterate_start (signature)
    {
      value = (value << 5) - value + signature_char;
    }
  g_signature_iterate_end;

  return value;
}

/**
 * g_signature_equal:
 * @signature1: a #GSignature
 * @signature2: a #GSignature
 * @returns: %TRUE if @signature1 and @signature2
 *           are exactly equal
 *
 * Determines if two signatures are exactly equal.
 * If one signature is non-concrete then the other
 * must also be non-concrete (in exactly the same
 * way).
 */
gboolean
g_signature_equal (gconstpointer signature1,
                   gconstpointer signature2)
{
  g_signature_iterate_together (signature1, signature2) {
  } g_signature_iterate_end;

  return TRUE;
}

/**
 * g_signature_match:
 * @pattern: a #GSignature
 * @signature: a #GSignature
 * @returns: %TRUE if @signature matches @patten
 *
 * Determines if @pattern and @signature form a
 * match.
 *
 * A match is formed if the wildcard positions
 * in @pattern can be replaced with valid
 * signatures to form a new signature that is
 * exactly equal to @signature.
 *
 * The replacements may be with any other valid
 * signature type, including abstract signatures,
 * including %G_SIGNATURE_ANY.
 *
 * For example, given a @pattern of "a{s*}", the
 * following signatures all match: "a{s*}",
 * "a{si}", "a{s(i*)}", "a{sai}", "a{sv}".
 *
 * It is valid to call this function with a
 * concrete #GSignature for @pattern (in which
 * case it behaves exactly like
 * g_signature_equal()).
 **/
gboolean
g_signature_matches (GSignature signature,
                     GSignature pattern)
{
  g_signature_iterate_together (pattern, signature)
  {
    case '*':
      signature_str = g_signature_iterate (signature_str - 1);
  }
  g_signature_iterate_end;

  return TRUE;
}

/**
 * g_signature_element:
 * @signature: an array #GSignature
 * @returns: the element #GSignature
 *
 * Determines the element type of an array or
 * a maybe #GSignature.
 *
 * The result must -not- be freed and remains
 * valid so long as the original @signature
 * remains valid.
 *
 * It is an error to call this function with a
 * @signature that has a type other than
 * %G_SIGNATURE_TYPE_ARRAY or
 * %G_SIGNATURE_TYPE_MAYBE.
 **/
#undef g_signature_element
GSignature
g_signature_element (GSignature signature)
{
  return g_signature_open_blindly (signature);
}

/**
 * g_signature_first:
 * @signature: a struct #GSignature
 * @returns: the first item #GSignature
 *
 * Determines the type of the first item of a
 * structure #GSignature.  Note that all
 * structures contain at least one item.
 *
 * The result must -not- be freed and remains
 * valid so long as the original @signature
 * remains valid.
 *
 * It is an error to call this function with a
 * @signature that has a type other than
 * %G_SIGNATURE_TYPE_STRUCT.
 **/
GSignature
g_signature_first (GSignature signature)
{
  const char *signature_str = (const char *) signature;

  g_assert (*signature_str == '{' || *signature_str == '(');

  signature_str++;

  if (*signature_str == ')')
    signature_str = NULL;

  return (GSignature) signature_str;
}

/**
 * g_signature_next:
 * @signature: a struct #GSignature
 * @returns: the next item #GSignature of %NULL
 *
 * Determines the type of the next item of a
 * structure #GSignature.  If there are no more
 * items in the structure, return %NULL.
 *
 * The result must -not- be freed and remains
 * valid so long as the original @signature
 * remains valid.
 *
 * This function must only be called on a
 * #GSignature returned by g_signature_first() or
 * a prior call to g_signature_next().
 *
 * This call is intended to be used along with
 * g_signature_first() as part of an iterator
 * pattern:
 *
 * <programlisting>
 * GSignature item;
 *
 * // loop over the signatures of each structure member
 * for (item = g_signature_first (struct); item; item = g_signature_next (item))
 *   {
 *      ...
 *   }
 * </programlisting>
 **/
GSignature
g_signature_next (GSignature signature)
{
  const char *signature_str = (const char *) signature;

  signature_str = g_signature_iterate (signature_str);

  /* undocumented: we actually support dict-entries too,
   * for direct use with g_signature_open_blindly */
  if (*signature_str == ')' || *signature_str == '}')
    signature_str = NULL;

  return (GSignature) signature_str;
}

/**
 * g_signature_items:
 * @signature: a struct #GSignature
 * @returns: the number of items in the structure
 *
 * Determines the number of items in a structure
 * of the type given in @signature.
 **/
int
g_signature_items (GSignature signature)
{
  const char *signature_str = (const char *) signature;
  int count = 0;

  g_signature_assert (signature, STRUCT);

  signature_str++;

  while (*signature_str != ')')
  {
    signature_str = g_signature_iterate (signature_str);
    count++;
  }

  return count;
}

/**
 * g_signature_key:
 * @signature: a dict entry #GSignature
 *
 * Determines the type of the key of a
 * dictionary entry #GSignature.
 *
 * The result must -not- be freed and remains
 * valid so long as the original @signature
 * remains valid.
 *
 * It is an error to call this function with a
 * @signature that has a type other than
 * %G_SIGNATURE_TYPE_DICT_ENTRY.
 **/
#undef g_signature_key
GSignature
g_signature_key (GSignature signature)
{
  g_signature_assert (signature, DICT_ENTRY);

  return g_signature_open_blindly (signature);
}

/**
 * g_signature_value:
 * @signature: a dict entry #GSignature
 *
 * Determines the type of the value of a
 * dictionary entry #GSignature.
 *
 * The result must -not- be freed and remains
 * valid so long as the original @signature
 * remains valid.
 *
 * It is an error to call this function with a
 * @signature that has a type other than
 * %G_SIGNATURE_TYPE_DICT_ENTRY.
 **/
GSignature
g_signature_value (GSignature signature)
{
  g_signature_assert (signature, DICT_ENTRY);

  return g_signature_next (g_signature_open_blindly (signature));
}

/**
 * g_signature_arrayify:
 * @element: a #GSignature
 * @returns: an array #GSignature
 *
 * Forms the signature corresponding to an array
 * of elements of type @element.
 *
 * The result is a newly-allocated #GSignature
 * which must be freed with g_signature_free().
 **/
GSignature
g_signature_arrayify (GSignature element)
{
  int elemlen;
  char *tmp;
  int size;

  elemlen = g_signature_length (element);
  size = elemlen + 1;

  tmp = g_alloca (size + 1);
  tmp[0] = 'a';
  memcpy (&tmp[1], element, elemlen);
  tmp[size] = '\0';

  return g_signature_new (tmp);
}

/**
 * g_signature_maybeify:
 * @element: a #GSignature
 * @returns: a maybe #GSignature
 *
 * Forms the signature corresponding to a maybe
 * type containing either Nothing or an instance
 * of the type @element
 *
 * The result is a newly-allocated #GSignature
 * which must be freed with g_signature_free().
 **/
GSignature
g_signature_maybeify (GSignature element)
{
  int elemlen;
  char *tmp;
  int size;

  elemlen = g_signature_length (element);
  size = elemlen + 1;

  tmp = g_alloca (size + 1);
  tmp[0] = 'm';
  memcpy (&tmp[1], element, elemlen);
  tmp[size] = '\0';

  return g_signature_new (tmp);
}

/**
 * g_signature_structify:
 * @items: an array of #GSignature
 * @length: length of @items
 * @returns: a struct #GSignature
 *
 * Forms the signature corresponding to a
 * structure with items of the types from the
 * @items array.
 *
 * The result is a newly-allocated #GSignature
 * which must be freed with g_signature_free().
 **/
GSignature
g_signature_structify (GSignature *items,
                       int         length)
{
  char *tmp;
  int total;
  int i;

  total = 2;

  for (i = 0; i < length; i++)
    total += g_signature_length (items[i]);

  tmp = g_alloca (total + 1);
  tmp[0] = '(';
  tmp[total - 1] = ')';
  tmp[total] = '\0';

  total = 0;
  tmp[total++] = '(';
  for (i = 0; i < length; i++)
  {
    int itemlen = g_signature_length (items[i]);

    memcpy (&tmp[total], items[i], itemlen);
    total += itemlen;
  }
  tmp[total++] = ')';
  tmp[total] = '\0';

  return g_signature_new (tmp);
}

/**
 * g_signature_dictify:
 * @key: a #GSignature
 * @value: a #GSignature
 * @returns: a new dict entry #GSignature
 *
 * Forms the signature corresponding to a
 * dictionary entry with a key type of @key and a
 * value type of @value.
 *
 * The result is a newly-allocated #GSignature
 * which must be freed with g_signature_free().
 **/
GSignature
g_signature_dictify (GSignature key,
                     GSignature value)
{
  int keylen, valuelen;
  char *tmp;
  int size;

  keylen = g_signature_length (key);
  valuelen = g_signature_length (value);
  size = keylen + valuelen + 2;

  tmp = g_alloca (size + 1);
  tmp[0] = '{';
  memcpy (&tmp[1], key, keylen);
  memcpy (&tmp[keylen + 1], value, valuelen);
  tmp[size - 1] = '}';
  tmp[size] = '\0';

  return g_signature_new (tmp);
}

/**
 * g_signature_construct:
 * @pattern: a signature string
 * @...: a #GSignature for each '*'
 * @returns: a new #GSignature
 *
 * Constructs a new #GSignature from @pattern.
 * For each '*' appearing in sequence in @pattern,
 * an #GSignature argument is read from the
 * variable argument list and inserted at that
 * point.
 *
 * For example:
 *
 * <programlisting>
 * g_signature_construct ("a*", G_SIGNATURE_STRING);
 * </programlisting>
 *
 * is equivalent to
 *
 * <programlisting>
 * g_signature_new ("as");
 * </programlisting>
 *
 * If there is a #GSignature in the parameter list
 * with a wildcard position then the final result
 * will have a wildcard position.
 *
 * The return value must be freed with
 * g_signature_free().
 *
 * Is is an error to call this function with an
 * invalid signature string.
 **/
GSignature
g_signature_construct (const char *pattern,
                       ...)
{
  GSignature *args;
  int *lengths;
  char *result;
  va_list ap;
  int length;
  int stars;
  int i, j, k;

  g_assert (g_signature_is_valid (pattern));

  /* boring case #1 */
  if (pattern[0] == '*')
    {
      GSignature result;

      va_start (ap, pattern);
      result = g_signature_copy (va_arg (ap, GSignature));
      va_end (ap);

      return result;
    }

  stars = 0;
  for (i = 0; pattern[i]; i++)
    if (pattern[i] == '*')
      stars++;

  /* boring case #2 */
  if (stars == 0)
    return g_signature_new (pattern);

  length = i - stars;
  args = g_alloca (sizeof (GSignature) * stars);
  lengths = g_alloca (sizeof (int) * stars);

  va_start (ap, pattern);
  for (i = 0; i < stars; i++)
    {
      args[i] = va_arg (ap, GSignature);
      lengths[i] = g_signature_length (args[i]);
      length += lengths[i];
    }
  va_end (ap);

  result = g_malloc (length + 1);

  j = 0; k = 0;
  for (i = 0; pattern[i]; i++)
    if (pattern[i] != '*')
      result[j++] = pattern[i];
    else
      {
        memcpy (&result[j], args[k], lengths[k]);
        j += lengths[k++];
      }

  return (GSignature) result;
}

static gboolean
g_signature_destruct_internal (GSignature signature,
                               GSignature pattern,
                               va_list    ap)
{
  GSignature *arg;

  g_signature_iterate_together (pattern, signature)
  {
    case '*':
      arg = va_arg (ap, GSignature *);
      *arg = (GSignature) (signature_str - 1);

      signature_str = g_signature_iterate (signature_str - 1);
  }
  g_signature_iterate_end;

  return TRUE;
}

/**
 * g_signature_destruct:
 * @signature: a #GSignature
 * @pattern: a signature string
 * @...: a pointer to a #GSignature for each '*'
 * @returns: %TRUE if the destruction succeeded
 *
 * Performs the inverse operation of
 * g_signature_construct().
 *
 * If @signature matches @pattern then, for each
 * '*' character in @pattern, the matched type
 * within @signature is returned.  The returned
 * signatures do not need to be freed and remain
 * valid as long as @signature is valid.
 *
 * If @signature doesn't match @pattern then
 * %FALSE is returned.
 **/
gboolean
g_signature_destruct (GSignature  signature,
                      const char *pattern,
                      ...)
{
  gboolean result;
  va_list ap;

  g_assert (g_signature_is_valid (pattern));

  va_start (ap, pattern);
  result = g_signature_destruct_internal (signature,
                                          (GSignature) pattern,
                                          ap);
  va_end (ap);

  return result;
}

gboolean
g_signature_type_is_basic (GSignatureType type)
{
  switch (type)
  {
    case G_SIGNATURE_TYPE_ARRAY:
    case G_SIGNATURE_TYPE_MAYBE:
    case G_SIGNATURE_TYPE_STRUCT:
    case G_SIGNATURE_TYPE_DICT_ENTRY:
    case G_SIGNATURE_TYPE_VARIANT:
      return FALSE;

    default:
      return TRUE;
  }
}
