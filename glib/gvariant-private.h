/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#ifndef _gvariant_private_h_
#define _gvariant_private_h_

#include "gvariant-loadstore.h"
#include "gvarianttypeinfo.h"

/* gvariant-core.c */
GVariant                       *g_variant_new_tree                      (const GVariantType  *type,
                                                                         GVariant           **children,
                                                                         gsize                n_children,
                                                                         gboolean             trusted);
void                            g_variant_ensure_native_endian          (GVariant            *value);
void                            g_variant_assert_invariant              (GVariant            *value);
gboolean                        g_variant_is_normalised                 (GVariant            *value);
GVariant                       *g_variant_ensure_floating               (GVariant            *value);

#endif /* _gvariant_private_h_ */
