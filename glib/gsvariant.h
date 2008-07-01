/*
 * Copyright © 2008 Ryan Lortie
 *
 * All rights reserved.
 */

#ifndef _gsvariant_h_
#define _gsvariant_h_

#include "gsvhelper.h"

typedef struct
{
  GSVHelper *helper;
  guchar *data;
  gsize size;
} GSVariant;

typedef void  (*GSVariantFiller)              (GSVariant       *svalue,
                                               gpointer         data);

void            g_svariant_assert_invariant   (GSVariant        value);

gboolean        g_svariant_is_normalised      (GSVariant        value);
void            g_svariant_byteswap           (GSVariant        value);

gsize           g_svariant_needed_size        (GSVHelper       *helper,
                                               GSVariantFiller  gsv_filler,
                                               gpointer        *children,
                                               gsize            n_children);

void            g_svariant_serialise          (GSVariant        container,
                                               GSVariantFiller  gsv_filler,
                                               gpointer        *children,
                                               gsize            n_children);

GSVariant       g_svariant_get_child          (GSVariant        container,
                                               gsize            index);

gsize           g_svariant_n_children         (GSVariant        container);

static inline GSVariant
g_svariant (GSVHelper *helper,
            guchar    *data,
            gsize      size)
{
  GSVariant value = { helper, data, size };

  g_svariant_assert_invariant (value);

  return value;
}

#endif /* _gsvariant_h_ */
