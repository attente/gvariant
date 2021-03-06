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

#include <glib/gvarianttype.h>
#include <glib/gstring.h>
#include <glib/gmarkup.h>
#include <glib/gerror.h>
#include <stdarg.h>

typedef struct                  OPAQUE_TYPE__GVariant                   GVariant;
typedef struct                  OPAQUE_TYPE__GVariantIter               GVariantIter;
typedef struct                  OPAQUE_TYPE__GVariantBuilder            GVariantBuilder;

struct OPAQUE_TYPE__GVariantIter
{
  gpointer private[8];
};

#pragma GCC visibility push (default)

GVariant                       *g_variant_ref                           (GVariant             *value);
GVariant                       *g_variant_ref_sink                      (GVariant             *value);
void                            g_variant_unref                         (GVariant             *value);
void                            g_variant_flatten                       (GVariant             *value);

GVariantTypeClass               g_variant_get_type_class                (GVariant             *value);
const GVariantType             *g_variant_get_type                      (GVariant             *value);
const gchar                    *g_variant_get_type_string               (GVariant             *value);
gboolean                        g_variant_is_basic                      (GVariant             *value);
gboolean                        g_variant_is_container                  (GVariant             *value);
gboolean                        g_variant_matches                       (GVariant             *value,
                                                                         const GVariantType   *pattern);

/* varargs construct/deconstruct */
GVariant                       *g_variant_new                           (const gchar          *format_string,
                                                                         ...);
void                            g_variant_get                           (GVariant             *value,
                                                                         const gchar          *format_string,
                                                                         ...);

gboolean                        g_variant_format_string_scan            (const gchar         **format_string);
GVariant                       *g_variant_new_va                        (const gchar         **format_string,
                                                                         va_list              *app);
void                            g_variant_get_va                        (GVariant             *value,
                                                                         const gchar         **format_string,
                                                                         va_list              *app);

/* constructors */
GVariant                       *g_variant_new_boolean                   (gboolean              boolean);
GVariant                       *g_variant_new_byte                      (guint8                byte);
GVariant                       *g_variant_new_uint16                    (guint16               uint16);
GVariant                       *g_variant_new_int16                     (gint16                int16);
GVariant                       *g_variant_new_uint32                    (guint32               uint32);
GVariant                       *g_variant_new_int32                     (gint32                int32);
GVariant                       *g_variant_new_uint64                    (guint64               uint64);
GVariant                       *g_variant_new_int64                     (gint64                int64);
GVariant                       *g_variant_new_double                    (gdouble               floating);
GVariant                       *g_variant_new_string                    (const gchar          *string);
GVariant                       *g_variant_new_object_path               (const gchar          *string);
gboolean                        g_variant_is_object_path                (const gchar          *string);
GVariant                       *g_variant_new_signature                 (const gchar          *string);
gboolean                        g_variant_is_signature                  (const gchar          *string);
GVariant                       *g_variant_new_variant                   (GVariant             *value);

/* deconstructors */
gboolean                        g_variant_get_boolean                   (GVariant             *value);
guint8                          g_variant_get_byte                      (GVariant             *value);
guint16                         g_variant_get_uint16                    (GVariant             *value);
gint16                          g_variant_get_int16                     (GVariant             *value);
guint32                         g_variant_get_uint32                    (GVariant             *value);
gint32                          g_variant_get_int32                     (GVariant             *value);
guint64                         g_variant_get_uint64                    (GVariant             *value);
gint64                          g_variant_get_int64                     (GVariant             *value);
gdouble                         g_variant_get_double                    (GVariant             *value);
const gchar                    *g_variant_get_string                    (GVariant             *value,
                                                                         gsize                *length);
gchar                          *g_variant_dup_string                    (GVariant             *value,
                                                                         gsize                *length);
GVariant                       *g_variant_get_variant                   (GVariant             *value);
gconstpointer                   g_variant_get_fixed                     (GVariant             *value,
                                                                         gsize                 size);
gconstpointer                   g_variant_get_fixed_array               (GVariant             *value,
                                                                         gsize                 elem_size,
                                                                         gsize                *length);
GVariant                       *g_variant_get_child                     (GVariant             *value,
                                                                         gsize                 index);
gsize                           g_variant_n_children                    (GVariant             *value);

/* GVariantIter */
gsize                           g_variant_iter_init                     (GVariantIter         *iter,
                                                                         GVariant             *value);
GVariant                       *g_variant_iter_next                     (GVariantIter         *iter);
void                            g_variant_iter_cancel                   (GVariantIter         *iter);
gboolean                        g_variant_iter_was_cancelled            (GVariantIter         *iter);
gboolean                        g_variant_iterate                       (GVariantIter         *iter,
                                                                         const gchar          *format_string,
                                                                         ...);

/* GVariantBuilder */
void                            g_variant_builder_add_value             (GVariantBuilder      *builder,
                                                                         GVariant             *value);
void                            g_variant_builder_add                   (GVariantBuilder      *builder,
                                                                         const gchar          *format_string,
                                                                         ...);
GVariantBuilder                *g_variant_builder_open                  (GVariantBuilder      *parent,
                                                                         GVariantTypeClass     class,
                                                                         const GVariantType   *type);
GVariantBuilder                *g_variant_builder_close                 (GVariantBuilder      *child);
gboolean                        g_variant_builder_check_add             (GVariantBuilder      *builder,
                                                                         GVariantTypeClass     class,
                                                                         const GVariantType   *type,
                                                                         GError              **error);
gboolean                        g_variant_builder_check_end             (GVariantBuilder      *builder,
                                                                         GError              **error);
GVariantBuilder                *g_variant_builder_new                   (GVariantTypeClass     class,
                                                                         const GVariantType   *type);
GVariant                       *g_variant_builder_end                   (GVariantBuilder      *builder);
void                            g_variant_builder_cancel                (GVariantBuilder      *builder);

/* markup printing/parsing */
GString                        *g_variant_markup_print                  (GVariant             *value,
                                                                         GString              *string,
                                                                         gboolean              newlines,
                                                                         gint                  indentation,
                                                                         gint                  tabstop);
void                            g_variant_markup_subparser_start        (GMarkupParseContext  *context,
                                                                         const GVariantType   *type);
GVariant                       *g_variant_markup_subparser_end          (GMarkupParseContext  *context,
                                                                         GError              **error);
GMarkupParseContext            *g_variant_markup_parse_context_new      (GMarkupParseFlags     flags,
                                                                         const GVariantType   *type);
GVariant                       *g_variant_markup_parse_context_end      (GMarkupParseContext  *context,
                                                                         GError              **error);
GVariant                       *g_variant_markup_parse                  (const gchar          *text,
                                                                         gssize                text_len,
                                                                         const GVariantType   *type,
                                                                         GError              **error);

#pragma GCC visibility pop

#define G_VARIANT_BUILDER_ERROR \
    g_quark_from_static_string ("g-variant-builder-error-quark")

typedef enum
{
  G_VARIANT_BUILDER_ERROR_TOO_MANY,
  G_VARIANT_BUILDER_ERROR_TOO_FEW,
  G_VARIANT_BUILDER_ERROR_INFER,
  G_VARIANT_BUILDER_ERROR_TYPE
} GVariantBuilderError;

#endif /* _gvariant_h_ */
