/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#ifndef _gsignature_h_
#define _gsignature_h_

#include <glib/gtypes.h>

typedef struct   OPAQUE_TYPE_GSignature   *GSignature;

/** GSignatureType: @G_SIGNATURE_TYPE_INVALID: the type of no
 * signature.  This can be used as an 'error' return value.
 *
 * @G_SIGNATURE_TYPE_BOOLEAN: the type of the signature
 * %G_SIGNATURE_BOOLEAN
 * @G_SIGNATURE_TYPE_BYTE: the type of the signature %G_SIGNATURE_BYTE
 * @G_SIGNATURE_TYPE_INT16: the type of the signature
 * %G_SIGNATURE_INT16
 * @G_SIGNATURE_TYPE_UINT16: the type of the signature
 * %G_SIGNATURE_UINT16
 * @G_SIGNATURE_TYPE_INT32: the type of the signature
 * %G_SIGNATURE_INT32
 * @G_SIGNATURE_TYPE_UINT32: the type of the signature
 * %G_SIGNATURE_UINT32
 * @G_SIGNATURE_TYPE_INT64: the type of the signature
 * %G_SIGNATURE_INT64
 * @G_SIGNATURE_TYPE_UINT64: the type of the signature
 * %G_SIGNATURE_UINT64
 * @G_SIGNATURE_TYPE_DOUBLE: the type of the signature
 * %G_SIGNATURE_DOUBLE
 * @G_SIGNATURE_TYPE_STRING: the type of the signature
 * %G_SIGNATURE_STRING
 * @G_SIGNATURE_TYPE_OBJECT_PATH: the type of the signature
 * %G_SIGNATURE_OBJECT_PATH
 * @G_SIGNATURE_TYPE_SIGNATURE: the type of the signature
 * %G_SIGNATURE_SIGNATURE
 * @G_SIGNATURE_TYPE_VARIANT: the type of the signature
 * %G_SIGNATURE_VARIANT
 * @G_SIGNATURE_TYPE_MAYBE: the type of any signature that is a maybe
 * signature.
 * @G_SIGNATURE_TYPE_ARRAY: the type of any signature that is an
 * array.
 * @G_SIGNATURE_TYPE_STRUCT: the type of any signature that is a
 * structure.
 * @G_SIGNATURE_TYPE_DICT_ENTRY: the type of any signature that is a
 * dictionary entry.
 * @G_SIGNATURE_TYPE_ANY: the type of exactly one signature:
 * %G_SIGNATURE_ANY
 *
 * A enumerated type to group #GSignature instances into categories.
 * If you ever want to perform some sort of recursive operation on the
 * contents of a #GSignature you will probably end up using a switch
 * statement over the #GSignatureType of the signature and its
 * component sub-signatures.
 *
 * A #GSignature is said to "have" a given #GSignatureType.  A
 * #GSignature has only one type.  For example, the
 * @G_SIGNATURE_TYPE_ANY is not a type of every #GSignature.
 **/


typedef enum
{
  G_SIGNATURE_TYPE_INVALID           = '\0',
  G_SIGNATURE_TYPE_BOOLEAN            = 'b',
  G_SIGNATURE_TYPE_BYTE               = 'y',

  G_SIGNATURE_TYPE_INT16              = 'n',
  G_SIGNATURE_TYPE_UINT16             = 'q',
  G_SIGNATURE_TYPE_INT32              = 'i',
  G_SIGNATURE_TYPE_UINT32             = 'u',
  G_SIGNATURE_TYPE_INT64              = 'x',
  G_SIGNATURE_TYPE_UINT64             = 't',

  G_SIGNATURE_TYPE_DOUBLE             = 'd',

  G_SIGNATURE_TYPE_STRING             = 's',
  G_SIGNATURE_TYPE_OBJECT_PATH        = 'o',
  G_SIGNATURE_TYPE_SIGNATURE          = 'g',

  G_SIGNATURE_TYPE_VARIANT            = 'v',

  G_SIGNATURE_TYPE_MAYBE              = 'm',
  G_SIGNATURE_TYPE_ARRAY              = 'a',
  G_SIGNATURE_TYPE_STRUCT             = 'r',
  G_SIGNATURE_TYPE_DICT_ENTRY         = 'e',

  G_SIGNATURE_TYPE_ANY                = '*'
} GSignatureType;

/**
 * G_SIGNATURE_BOOLEAN:
 *
 * The type of a value that can be either %TRUE or %FALSE.
 **/
#define G_SIGNATURE_BOOLEAN              ((GSignature) "b")

/**
 * G_SIGNATURE_BYTE:
 *
 * The type of an integer value that can range from 0 to 255.
 **/
#define G_SIGNATURE_BYTE                 ((GSignature) "y")

/**
 * G_SIGNATURE_INT16:
 *
 * The type of an integer value that can range from -32768 to 32767.
 **/
#define G_SIGNATURE_INT16                ((GSignature) "n")

/**
 * G_SIGNATURE_UINT16:
 *
 * The type of an integer value that can range from 0 to 65535.
 * If you had a compass, a ruler and an awful lot of patience, you
 * could construct a regular polygon with 65535 sides.
 **/
#define G_SIGNATURE_UINT16               ((GSignature) "q")

/**
 * G_SIGNATURE_INT32:
 *
 * The type of an integer value that can range from -2147483648 to
 * 2147483647.
 **/
#define G_SIGNATURE_INT32                ((GSignature) "i")

/**
 * G_SIGNATURE_UINT32:
 *
 * The type of an integer value that can range from 0 to 4294967295.
 * That's one number for everyone who was around in the late 1970s.
 **/
#define G_SIGNATURE_UINT32               ((GSignature) "u")

/**
 * G_SIGNATURE_INT64:
 *
 * The type of an integer value that can range from
 * -9223372036854775808 to 9223372036854775807.
 **/
#define G_SIGNATURE_INT64                ((GSignature) "x")

/**
 * G_SIGNATURE_UINT64:
 *
 * The type of an integer value that can range from 0 to
 * 18446744073709551616.  That's a really big number, but a Rubik's
 * cube can have a bit more than twice as many possible positions.
 **/
#define G_SIGNATURE_UINT64               ((GSignature) "t")

/**
 * G_SIGNATURE_DOUBLE:
 *
 * The type of a double precision IEEE754 floating point number.
 * These guys go up to about 1.80e308 but miss out on some numbers in
 * between.  In any case, that's far greater than the estimated number
 * of fundamental particles in the observable universe.
 **/
#define G_SIGNATURE_DOUBLE               ((GSignature) "d")

/**
 * G_SIGNATURE_STRING:
 *
 * The type of a string.  "" is a string.  %NULL is not a string.
 **/
#define G_SIGNATURE_STRING               ((GSignature) "s")

/**
 * G_SIGNATURE_OBJECT_PATH:
 *
 * The type of a DBus object reference.  These are strings of a
 * specific format used to identify objects at a given destination on
 * the bus.
 **/
#define G_SIGNATURE_OBJECT_PATH          ((GSignature) "o")

/**
 * G_SIGNATURE_SIGNATURE:
 *
 * The type of a DBus type signature.  These are strings of a specific
 * format used as type signatures for DBus methods and messages.
 *
 * Any valid #GSignature signature string is a valid DBus type signature.
 * In addition, a concatenation of any number of valid #GSignature
 * signature strings is also a valid DBus type signature.
 **/
#define G_SIGNATURE_SIGNATURE            ((GSignature) "g")

/**
 * G_SIGNATURE_VARIANT:
 *
 * The type of a box that contains any other value (including another
 * variant).
 **/
#define G_SIGNATURE_VARIANT              ((GSignature) "v")

/**
 * G_SIGNATURE_UNIT:
 *
 * The empty structure type.  Has only one valid instance.
 **/
#define G_SIGNATURE_UNIT                 ((GSignature) "()")

/**
 * G_SIGNATURE_ANY:
 *
 * The wildcard type.  Matches any type.
 **/
#define G_SIGNATURE_ANY                  ((GSignature) "*")

/**
 * G_SIGNATURE_ANY_MAYBE:
 *
 * A wildcard type matching any maybe type.
 **/
#define G_SIGNATURE_ANY_MAYBE            ((GSignature) "m*")

/**
 * G_SIGNATURE_ANY_ARRAY:
 *
 * A wildcard type matching any array type.
 **/
#define G_SIGNATURE_ANY_ARRAY            ((GSignature) "a*")

/**
 * G_SIGNATURE_ANY_STRUCT:
 *
 * A wildcard type matching any structure type.
 **/
#define G_SIGNATURE_ANY_STRUCT           ((GSignature) "r")

/**
 * G_SIGNATURE_ANY_DICT_ENTRY:
 *
 * A wildcard type matching any dictionary entry type.
 **/
#define G_SIGNATURE_ANY_DICT_ENTRY       ((GSignature) "e") /* aka "{**}" */

/**
 * G_SIGNATURE_ANY_DICTIONARY:
 *
 * A wildcard type matching any dictionary type.
 **/
#define G_SIGNATURE_ANY_DICTIONARY       ((GSignature) "ae")

#pragma GCC visibility push (default)

void           g_signature_free      (GSignature      signature);
GSignature     g_signature_copy      (GSignature      signature) G_GNUC_MALLOC;

gboolean       g_signature_is_valid  (const char     *string)      G_GNUC_PURE;
GSignature     g_signature_new       (const char     *string)    G_GNUC_MALLOC;
GSignature     g_signature           (const char     *string)      G_GNUC_PURE;

int            g_signature_length    (GSignature      signature)   G_GNUC_PURE;
const char    *g_signature_peek      (GSignature      signature)   G_GNUC_PURE;
char          *g_signature_get       (GSignature      signature) G_GNUC_MALLOC;

gboolean       g_signature_concrete  (GSignature      signature)   G_GNUC_PURE;
gboolean       g_signature_equal     (gconstpointer   signature1,
                                      gconstpointer   signature2)  G_GNUC_PURE;
gboolean       g_signature_matches   (GSignature      signature,
                                      GSignature      pattern)     G_GNUC_PURE;
gboolean       g_signature_has_type  (GSignature      signature,
                                      GSignatureType  type)        G_GNUC_PURE;
GSignatureType g_signature_type      (GSignature      signature)   G_GNUC_PURE;

GSignature     g_signature_element   (GSignature      signature)   G_GNUC_PURE;
GSignature     g_signature_first     (GSignature      signature)   G_GNUC_PURE;
GSignature     g_signature_next      (GSignature      signature)   G_GNUC_PURE;
int            g_signature_items     (GSignature      signature)   G_GNUC_PURE;
GSignature     g_signature_key       (GSignature      signature)   G_GNUC_PURE;
GSignature     g_signature_value     (GSignature      signature)   G_GNUC_PURE;

GSignature     g_signature_construct (const char     *pattern,
                                      ...)                       G_GNUC_MALLOC;
gboolean       g_signature_destruct  (GSignature      signature,
                                      const char     *pattern,
                                      ...);
guint          g_signature_hash      (gconstpointer   signature) G_GNUC_PURE;

GSignature     g_signature_arrayify  (GSignature      element)   G_GNUC_MALLOC;
GSignature     g_signature_maybeify  (GSignature      element)   G_GNUC_MALLOC;
GSignature     g_signature_structify (GSignature     *items,
                                      int             length)    G_GNUC_MALLOC;
GSignature     g_signature_dictify   (GSignature      key,
                                      GSignature      value)     G_GNUC_MALLOC;
gboolean       g_signature_type_is_basic (GSignatureType type) G_GNUC_CONST;

#pragma GCC visibility pop

#define g_signature_open_blindly(s)      ((GSignature) &((char *)(s))[1])

#ifdef G_SIGNATURE_DISABLE_CHECKS
#define g_signature_key(s)               (g_signature_open_blindly(s))
#define g_signature_element(s)           (g_signature_open_blindly(s))

#define g_signature_assert(sig, type)    do {} while (0)
#define g_signature(str)                 ((GSignature) (str))
#else
#define g_signature_assert(sig, type)    g_assert (g_signature_has_type (sig, \
                                          G_SIGNATURE_TYPE_##type))
#endif

#endif /* _gsignature_h_ */
