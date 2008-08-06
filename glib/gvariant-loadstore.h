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
  G_VARIANT_TRUSTED             = 1,
  G_VARIANT_BYTESWAP_NOW        = 2,
  G_VARIANT_BYTESWAP_LAZY       = 4,
  G_VARIANT_NORMALISE           = 8,

  G_VARIANT_EMBED_SIGNATURE     = 16
} GVariantFlags;

GVariant                       *g_variant_load                          (const GVariantType *type,
                                                                         gconstpointer       data,
                                                                         gsize               size,
                                                                         GVariantFlags       flags);
GVariant                       *g_variant_from_slice                    (const GVariantType *type,
                                                                         gpointer            slice,
                                                                         gsize               size,
                                                                         GVariantFlags       flags);

void                            g_variant_store                         (GVariant           *value,
                                                                         gpointer            data);
gconstpointer                   g_variant_get_data                      (GVariant           *value);
gsize                           g_variant_get_size                      (GVariant           *value);

void                            g_variant_normalise                     (GVariant           *value);

#pragma GCC visibility pop

#endif /* _gvariant_loadstore_h_ */
