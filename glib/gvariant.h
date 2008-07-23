/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#ifndef _gvariant_h_
#define _gvariant_h_

#include <glib/gsignature.h>
#include <glib/gstring.h>
#include <glib/gmarkup.h>
#include <glib/gerror.h>
#include <stdarg.h>

typedef struct OPAQUE_TYPE__GVariant        GVariant;
typedef struct OPAQUE_TYPE__GVariantIter    GVariantIter;
typedef struct OPAQUE_TYPE__GVariantBuilder GVariantBuilder;

struct OPAQUE_TYPE__GVariantIter
{
  gpointer private[4];
};

#pragma GCC visibility push (default)

GSignature      g_variant_get_signature   (GVariant        *value);
gboolean        g_variant_matches         (GVariant        *value,
                                           GSignature       pattern);
GVariant       *g_variant_ref             (GVariant        *value);
GVariant       *g_variant_unref           (GVariant        *value);

GVariant       *g_variant_vvnew           (gboolean         steal,
                                           GSignature       signature,
                                           va_list         *app);

GVariant       *g_variant_vnew            (gboolean         steal,
                                           GSignature       signature,
                                           va_list          ap);

GVariant       *g_variant_new_full        (gboolean         steal,
                                           GSignature       signature,
                                           ...);

GVariant       *g_variant_new             (const char      *signature_string,
                                           ...);

void            g_variant_vvget           (GVariant        *value,
                                           gboolean         free,
                                           GSignature       signature,
                                           va_list         *app);

void            g_variant_vget            (GVariant        *value,
                                           gboolean         free,
                                           GSignature       signature,
                                           va_list          ap);

void            g_variant_get_full        (GVariant        *value,
                                           gboolean         free,
                                           GSignature       signature,
                                           ...);

void            g_variant_get             (GVariant        *value,
                                           const char      *signature_string,
                                           ...);

void            g_variant_flatten         (GVariant        *value);
GVariant       *g_variant_new_boolean     (gboolean         boolean);
GVariant       *g_variant_new_byte        (guint8           byte);
GVariant       *g_variant_new_uint16      (guint16          uint16);
GVariant       *g_variant_new_int16       (gint16           int16);
GVariant       *g_variant_new_uint32      (guint32          uint32);
GVariant       *g_variant_new_int32       (gint32           int32);
GVariant       *g_variant_new_uint64      (guint64          uint64);
GVariant       *g_variant_new_int64       (gint64           int64);
GVariant       *g_variant_new_double      (gdouble          floating);
GVariant       *g_variant_new_string      (const char      *string);
GVariant       *g_variant_new_object_path (const char      *string);
GVariant       *g_variant_new_signature   (const char      *string);
GVariant       *g_variant_new_variant     (GVariant        *value);

gboolean        g_variant_get_boolean     (GVariant        *value);
guint8          g_variant_get_byte        (GVariant        *value);
guint16         g_variant_get_uint16      (GVariant        *value);
gint16          g_variant_get_int16       (GVariant        *value);
guint32         g_variant_get_uint32      (GVariant        *value);
gint32          g_variant_get_int32       (GVariant        *value);
guint64         g_variant_get_uint64      (GVariant        *value);
gint64          g_variant_get_int64       (GVariant        *value);
gdouble         g_variant_get_double      (GVariant        *value);
const gchar    *g_variant_get_string      (GVariant        *value,
                                           gsize           *length);
gchar          *g_variant_dup_string      (GVariant        *value,
                                           int             *length);
GVariant       *g_variant_get_variant     (GVariant        *value);

gconstpointer   g_variant_get_fixed_array (GVariant        *value,
                                           gsize           *n_items);
GVariant       *g_variant_get_child       (GVariant        *value,
                                           gsize            index);
gsize           g_variant_n_children      (GVariant        *value);


gboolean        g_variant_iterate         (GVariantIter    *iter,
                                           const char      *signature_string,
                                           ...);

GVariant       *g_variant_iter_next       (GVariantIter    *iter);

gint            g_variant_iter_init       (GVariantIter    *iter,
                                           GVariant        *value);

void            g_variant_iter_cancel     (GVariantIter    *iter);



GString        *g_variant_markup_print    (GVariant        *value,
                                           GString         *string,
                                           gboolean         newlines,
                                           gint             indentation,
                                           gint             tabstop);

void            g_variant_markup_parser_start_parse
                                          (GMarkupParseContext  *context,
                                           GSignature            signature);
GVariant       *g_variant_markup_parser_end_parse
                                          (GMarkupParseContext  *context,
                                           GError              **error);
GVariant       *g_variant_markup_parse    (const char           *string,
                                           GSignature            signature,
                                           GError              **error);

#define G_VARIANT_BUILDER_ERROR \
    g_quark_from_static_string ("g-variant-builder-error-quark")

typedef enum
{
  G_VARIANT_BUILDER_ERROR_TOO_MANY,
  G_VARIANT_BUILDER_ERROR_TOO_FEW,
  G_VARIANT_BUILDER_ERROR_CANNOT_INFER,
  G_VARIANT_BUILDER_ERROR_DOES_NOT_FIT
} GVariantBuilderError;

void             g_variant_builder_add_value    (GVariantBuilder  *builder,
                                                 GVariant         *value);
void             g_variant_builder_add          (GVariantBuilder  *builder,
                                                 const char       *signature,
                                                 ...);
GVariantBuilder *g_variant_builder_open         (GVariantBuilder  *parent,
                                                 GSignatureType    type,
                                                 GSignature        signature);
GVariantBuilder *g_variant_builder_close        (GVariantBuilder  *child);

gboolean         g_variant_builder_check_add    (GVariantBuilder  *builder,
                                                 GSignatureType    type,
                                                 GSignature        signature,
                                                 GError          **error);
gboolean         g_variant_builder_check_end    (GVariantBuilder  *builder,
                                                 GError          **error);

GVariantBuilder *g_variant_builder_new          (GSignatureType    type,
                                                 GSignature        signature);

GVariant        *g_variant_builder_end          (GVariantBuilder  *builder);
void             g_variant_builder_abort        (GVariantBuilder  *builder);


/* ugh... */
#undef va_ref
#ifdef G_VA_COPY_AS_ARRAY
#  define va_ref(x) ((va_list *) x)
#else
#  define va_ref(x) (&(x))
#endif

#pragma GCC visibility pop

#endif /* _gvariant_h_ */
