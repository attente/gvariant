/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#ifndef _gvariant_loadstore_h_
#define _gvariant_loadstore_h_

#include <glib/gvariant.h>

#pragma GCC visibility push (default)

typedef enum
{
  G_VARIANT_TRUSTED             = 0x00010000,
  G_VARIANT_LAZY_BYTESWAP       = 0x00020000,
} GVariantFlags;

GVariant                       *g_variant_load                          (const GVariantType *type,
                                                                         gconstpointer       data,
                                                                         gsize               size,
                                                                         GVariantFlags       flags);
GVariant                       *g_variant_from_slice                    (const GVariantType *type,
                                                                         gpointer            slice,
                                                                         gsize               size,
                                                                         GVariantFlags       flags);
GVariant                       *g_variant_from_data                     (const GVariantType *type,
                                                                         gconstpointer       data,
                                                                         gsize               size,
                                                                         GVariantFlags       flags,
                                                                         GDestroyNotify      notify,
                                                                         gpointer            user_data);

void                            g_variant_store                         (GVariant           *value,
                                                                         gpointer            data);
gconstpointer                   g_variant_get_data                      (GVariant           *value);
gsize                           g_variant_get_size                      (GVariant           *value);

void                            g_variant_normalise                     (GVariant           *value);

#pragma GCC visibility pop

#endif /* _gvariant_loadstore_h_ */
