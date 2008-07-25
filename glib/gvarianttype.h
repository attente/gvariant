/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#ifndef _gvarianttype_h_
#define _gvarianttype_h_

#include <glib/gtypes.h>

typedef struct   OPAQUE_TYPE_GVariantType   GVariantType;

/**
 * GVariantTypeClass:
 *
 * @G_VARIANT_TYPE_CLASS_INVALID: the class of no type
 * @G_VARIANT_TYPE_CLASS_BOOLEAN: the class containing the type %G_VARIANT_TYPE_BOOLEAN
 * @G_VARIANT_TYPE_CLASS_BYTE: the class containing the type %G_VARIANT_TYPE_BYTE
 * @G_VARIANT_TYPE_CLASS_INT16: the class containing the type %G_VARIANT_TYPE_INT16
 * @G_VARIANT_TYPE_CLASS_UINT16: the class containing the type %G_VARIANT_TYPE_UINT16
 * @G_VARIANT_TYPE_CLASS_INT32: the class containing the type %G_VARIANT_TYPE_INT32
 * @G_VARIANT_TYPE_CLASS_UINT32: the class containing the type %G_VARIANT_TYPE_UINT32
 * @G_VARIANT_TYPE_CLASS_INT64: the class containing the type %G_VARIANT_TYPE_INT64
 * @G_VARIANT_TYPE_CLASS_UINT64: the class containing the type %G_VARIANT_TYPE_UINT64
 * @G_VARIANT_TYPE_CLASS_DOUBLE: the class containing the type %G_VARIANT_TYPE_DOUBLE
 * @G_VARIANT_TYPE_CLASS_STRING: the class containing the type %G_VARIANT_TYPE_STRING
 * @G_VARIANT_TYPE_CLASS_OBJECT_PATH: the class containing the type %G_VARIANT_TYPE_OBJECT_PATH
 * @G_VARIANT_TYPE_CLASS_SIGNATURE: the class containing the type %G_VARIANT_TYPE_SIGNATURE
 * @G_VARIANT_TYPE_CLASS_VARIANT: the class containing the type %G_VARIANT_TYPE_VARIANT
 * @G_VARIANT_TYPE_CLASS_MAYBE: the class containing all maybe types
 *                              (any type matching %G_VARIANT_TYPE_ANY_MAYBE)
 * @G_VARIANT_TYPE_CLASS_ARRAY: the class containing all array types
 *                              (any type matching %G_VARIANT_TYPE_ANY_ARRAY)
 * @G_VARIANT_TYPE_CLASS_STRUCT: the class containing all structure types
 *                               (any type matching %G_VARIANT_TYPE_ANY_STRUCT)
 * @G_VARIANT_TYPE_CLASS_DICT_ENTRY: the class containing all dictionary entry types
 *                                   (any type matching %G_VARIANT_TYPE_ANY_DICT_ENTRY)
 * @G_VARIANT_TYPE_CLASS_ALL: the class containing all types
 *                            (any type matching %G_VARIANT_TYPE_ANY)
 * @G_VARIANT_TYPE_CLASS_BASIC: the class containing all basic types
 *                              (any type matching %G_VARIANT_TYPE_BASIC)
 * @G_VARIANT_TYPE_CLASS_FIXED: the class containing all fixed-size types
 *                              (any type matching %G_VARIANT_TYPE_FIXED)
 * @G_VARIANT_TYPE_CLASS_FIXED_BASIC: the class containing all fixed-size basic types
 *                                    (any type matching %G_VARIANT_TYPE_FIXED_BASIC)
 *
 * A enumerated type to group #GVariantType instances into classes.
 *
 * If you ever want to perform some sort of recursive operation on the
 * contents of a #GVariantType you will probably end up using a switch
 * statement over the #GVariantTypeClass of the type and its component
 * sub-types.
 *
 * A #GVariantType is said to "be of" a given #GVariantTypeClass.  A
 * #GVariantType can have several different type classes.  For
 * example, %G_VARIANT_TYPE_BOOLEAN is of the following classes:
 * %G_VARIANT_TYPE_CLASS_BOOLEAN, %G_VARIANT_TYPE_CLASS_BASIC,
 * %G_VARIANT_TYPE_CLASS_FIXED, %G_VARIANT_TYPE_CLASS_FIXED_BASIC,
 * %G_VARIANT_TYPE_CLASS_ALL.
 *
 * Each #GVariantType has one class that it is a member of that is
 * said to be the "natural class" of that type.  This is the "most
 * obvious" class.  The natural class of any basic type is the class
 * that contains only that type.  The natural class of any container
 * type is the class for that container.  The "fixed", "basic" or
 * "all" classes are the natural classes only for each of their
 * directly corresponding types.  An easy way to think of the natural
 * class of a type is that it can be completely determined from the
 * first character of the type string of that type.
 **/
typedef enum
{
  G_VARIANT_TYPE_CLASS_INVALID           = '\0',
  G_VARIANT_TYPE_CLASS_BOOLEAN            = 'b',
  G_VARIANT_TYPE_CLASS_BYTE               = 'y',

  G_VARIANT_TYPE_CLASS_INT16              = 'n',
  G_VARIANT_TYPE_CLASS_UINT16             = 'q',
  G_VARIANT_TYPE_CLASS_INT32              = 'i',
  G_VARIANT_TYPE_CLASS_UINT32             = 'u',
  G_VARIANT_TYPE_CLASS_INT64              = 'x',
  G_VARIANT_TYPE_CLASS_UINT64             = 't',

  G_VARIANT_TYPE_CLASS_DOUBLE             = 'd',

  G_VARIANT_TYPE_CLASS_STRING             = 's',
  G_VARIANT_TYPE_CLASS_OBJECT_PATH        = 'o',
  G_VARIANT_TYPE_CLASS_SIGNATURE          = 'g',

  G_VARIANT_TYPE_CLASS_VARIANT            = 'v',

  G_VARIANT_TYPE_CLASS_MAYBE              = 'm',
  G_VARIANT_TYPE_CLASS_ARRAY              = 'a',
  G_VARIANT_TYPE_CLASS_STRUCT             = 'r',
  G_VARIANT_TYPE_CLASS_DICT_ENTRY         = 'e',

  G_VARIANT_TYPE_CLASS_BASIC              = '?',
  G_VARIANT_TYPE_CLASS_FIXED              = '@',
  G_VARIANT_TYPE_CLASS_FIXED_BASIC        = '&',

  G_VARIANT_TYPE_CLASS_ALL                = '*',
} GVariantTypeClass;

/**
 * G_VARIANT_TYPE_BOOLEAN:
 *
 * The type of a value that can be either %TRUE or %FALSE.
 **/
#define G_VARIANT_TYPE_BOOLEAN              ((GVariantType) "b")

/**
 * G_VARIANT_TYPE_BYTE:
 *
 * The type of an integer value that can range from 0 to 255.
 **/
#define G_VARIANT_TYPE_BYTE                 ((GVariantType) "y")

/**
 * G_VARIANT_TYPE_INT16:
 *
 * The type of an integer value that can range from -32768 to 32767.
 **/
#define G_VARIANT_TYPE_INT16                ((GVariantType) "n")

/**
 * G_VARIANT_TYPE_UINT16:
 *
 * The type of an integer value that can range from 0 to 65535.
 * If you had a compass, a ruler and an awful lot of patience, you
 * could construct a regular polygon with 65535 sides.
 **/
#define G_VARIANT_TYPE_UINT16               ((GVariantType) "q")

/**
 * G_VARIANT_TYPE_INT32:
 *
 * The type of an integer value that can range from -2147483648 to
 * 2147483647.
 **/
#define G_VARIANT_TYPE_INT32                ((GVariantType) "i")

/**
 * G_VARIANT_TYPE_UINT32:
 *
 * The type of an integer value that can range from 0 to 4294967295.
 * That's one number for everyone who was around in the late 1970s.
 **/
#define G_VARIANT_TYPE_UINT32               ((GVariantType) "u")

/**
 * G_VARIANT_TYPE_INT64:
 *
 * The type of an integer value that can range from
 * -9223372036854775808 to 9223372036854775807.
 **/
#define G_VARIANT_TYPE_INT64                ((GVariantType) "x")

/**
 * G_VARIANT_TYPE_UINT64:
 *
 * The type of an integer value that can range from 0 to
 * 18446744073709551616.  That's a really big number, but a Rubik's
 * cube can have a bit more than twice as many possible positions.
 **/
#define G_VARIANT_TYPE_UINT64               ((GVariantType) "t")

/**
 * G_VARIANT_TYPE_DOUBLE:
 *
 * The type of a double precision IEEE754 floating point number.
 * These guys go up to about 1.80e308 but miss out on some numbers in
 * between.  In any case, that's far greater than the estimated number
 * of fundamental particles in the observable universe.
 **/
#define G_VARIANT_TYPE_DOUBLE               ((GVariantType) "d")

/**
 * G_VARIANT_TYPE_STRING:
 *
 * The type of a string.  "" is a string.  %NULL is not a string.
 **/
#define G_VARIANT_TYPE_STRING               ((GVariantType) "s")

/**
 * G_VARIANT_TYPE_OBJECT_PATH:
 *
 * The type of a DBus object reference.  These are strings of a
 * specific format used to identify objects at a given destination on
 * the bus.
 **/
#define G_VARIANT_TYPE_OBJECT_PATH          ((GVariantType) "o")

/**
 * G_VARIANT_TYPE_SIGNATURE:
 *
 * The type of a DBus type signature.  These are strings of a specific
 * format used as type signatures for DBus methods and messages.
 *
 * Any valid #GVariantType signature string is a valid DBus type
 * signature.  In addition, a concatenation of any number of valid
 * #GVariantType signature strings is also a valid DBus type
 * signature.
 **/
#define G_VARIANT_TYPE_SIGNATURE            ((GVariantType) "g")

/**
 * G_VARIANT_TYPE_VARIANT:
 *
 * The type of a box that contains any other value (including another
 * variant).
 **/
#define G_VARIANT_TYPE_VARIANT              ((GVariantType) "v")

/**
 * G_VARIANT_TYPE_UNIT:
 *
 * The empty structure type.  Has only one valid instance.
 **/
#define G_VARIANT_TYPE_UNIT                 ((GVariantType) "()")

/**
 * G_VARIANT_TYPE_ANY:
 *
 * The wildcard type.  Matches any type.
 **/
#define G_VARIANT_TYPE_ANY                  ((GVariantType) "*")

/**
 * G_VARIANT_TYPE_ANY_BASIC:
 *
 * A wildcard type matching any basic type.
 **/
#define G_VARIANT_TYPE_ANY_BASIC            ((GVariantType) "?")

/**
 * G_VARIANT_TYPE_ANY_MAYBE:
 *
 * A wildcard type matching any maybe type.
 **/
#define G_VARIANT_TYPE_ANY_MAYBE            ((GVariantType) "m*")

/**
 * G_VARIANT_TYPE_ANY_ARRAY:
 *
 * A wildcard type matching any array type.
 **/
#define G_VARIANT_TYPE_ANY_ARRAY            ((GVariantType) "a*")

/**
 * G_VARIANT_TYPE_ANY_STRUCT:
 *
 * A wildcard type matching any structure type.
 **/
#define G_VARIANT_TYPE_ANY_STRUCT           ((GVariantType) "r")

/**
 * G_VARIANT_TYPE_ANY_DICT_ENTRY:
 *
 * A wildcard type matching any dictionary entry type.
 **/
#define G_VARIANT_TYPE_ANY_DICT_ENTRY       ((GVariantType) "e")

/**
 * G_VARIANT_TYPE_ANY_DICTIONARY:
 *
 * A wildcard type matching any dictionary type.
 **/
#define G_VARIANT_TYPE_ANY_DICTIONARY       ((GVariantType) "ae")

#pragma GCC visibility push (default)

/* type string checking */
gboolean                        g_variant_type_string_is_valid          (const gchar         *type_string);
gboolean                        g_variant_type_string_scan              (const gchar        **type_string,
                                                                         const gchar         *limit);

/* create/destroy */
void                            g_variant_type_free                     (GVariantType        *type);
GVariantType                   *g_variant_type_copy                     (const GVariantType  *type);
GVariantType                   *g_variant_type_new                      (const gchar         *type_string);

/* getters */
gsize                           g_variant_type_get_string_length        (const GVariantType  *type);
const gchar                    *g_variant_type_peek_string              (const GVariantType  *type);
gchar                          *g_variant_type_dup_string               (const GVariantType  *type);

/* classification */
gboolean                        g_variant_type_is_concrete              (const GVariantType  *type);
gboolean                        g_variant_type_is_container             (const GVariantType  *type);
gboolean                        g_variant_type_is_basic                 (const GVariantType  *type);
gboolean                        g_variant_type_is_fixed_size            (const GVariantType  *type);

/* for hash tables */
guint                           g_variant_type_hash                     (gconstpointer        type);
gboolean                        g_variant_type_equal                    (gconstpointer        type1,
                                                                         gconstpointer        type2);

/* matching */
gboolean                        g_variant_type_matches                  (const GVariantType  *type,
                                                                         const GVariantType  *pattern);

/* class functions */
gboolean                        g_variant_type_is_of_class              (const GVariantType  *type,
                                                                         GVariantTypeClass    class);
GVariantTypeClass               g_variant_type_get_natural_class        (const GVariantType  *type);
gboolean                        g_variant_type_class_is_container       (GVariantTypeClass    class);
gboolean                        g_variant_type_class_is_basic           (GVariantTypeClass    class);

/* type iterator interface */
const GVariantType             *g_variant_type_element                  (const GVariantType  *type);
const GVariantType             *g_variant_type_first                    (const GVariantType  *type);
const GVariantType             *g_variant_type_next                     (const GVariantType  *type);
gsize                          *g_variant_type_n_items                  (const GVariantType  *type);
const GVariantType             *g_variant_type_key                      (const GVariantType  *type);
const GVariantType             *g_variant_type_value                    (const GVariantType  *type);

/* printf/scanf style */
GVariantType                   *g_variant_type_construct                (const GVariantType  *pattern,
                                                                         ...);
gboolean                        g_variant_type_destruct                 (const GVariantType  *type,
                                                                         const GVariantType  *pattern,
                                                                         ...);

/* constructors */
GVariantType                   *g_variant_type_new_array                (const GVariantType  *element);
GVariantType                   *g_variant_type_new_maybe                (const GVariantType  *element);
typedef const GVariantType   *(*GVariantTypeGetter)                     (gpointer             data);
GVariantType                   *g_variant_type_new_struct               (const gpointer      *items,
                                                                         gsize                length,
                                                                         GVariantTypeGetter   func);
GVariantType                   *g_variant_type_new_dict_entry           (const GVariantType  *key,
                                                                         const GVariantType  *value);

/*< private >*/
const GVariantType             *_g_variant_type_check_string            (const gchar         *type_string);

#pragma GCC visibility pop

#define G_VARIANT_TYPE(type_string) \
  (_g_variant_type_check_string (type_string))

#endif /* _gvarianttype_h_ */
