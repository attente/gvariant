/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#ifndef _gvariant_serialiser_h_
#define _gvariant_serialiser_h_

#include "gvarianttypeinfo.h"

typedef struct
{
  GVariantTypeInfo *type;
  guchar           *data;
  gsize             size;
} GVariantSerialised;

/* deserialisation */
gsize                           g_variant_serialised_n_children         (GVariantSerialised        container);
GVariantSerialised              g_variant_serialised_get_child          (GVariantSerialised        container,
                                                                         gsize                     index);

/* serialisation */
typedef void                  (*GVariantSerialisedFiller)               (GVariantSerialised       *serialised,
                                                                         gpointer                  data);

gsize                           g_variant_serialiser_needed_size        (GVariantTypeInfo         *info,
                                                                         GVariantSerialisedFiller  gsv_filler,
                                                                         const gpointer           *children,
                                                                         gsize                     n_children);

void                            g_variant_serialiser_serialise          (GVariantSerialised        container,
                                                                         GVariantSerialisedFiller  gsv_filler,
                                                                         const gpointer           *children,
                                                                         gsize                     n_children);

/* misc */
void                            g_variant_serialised_assert_invariant   (GVariantSerialised        value);
gboolean                        g_variant_serialised_is_normalised      (GVariantSerialised        value);
void                            g_variant_serialised_byteswap           (GVariantSerialised        value);

#endif /* _gvariant_serialiser_h_ */
