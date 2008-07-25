/*
 * Copyright © 2008 Ryan Lortie
 *
 * All rights reserved.
 */

#ifndef _gsvariant_h_
#define _gsvariant_h_

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

#if 0
static inline GSVariant
g_svariant (GSVHelper *helper,
            guchar    *data,
            gsize      size)
{
  GSVariant value = { helper, data, size };

  g_svariant_assert_invariant (value);

  return value;
}
#endif

#endif /* _gsvariant_h_ */
