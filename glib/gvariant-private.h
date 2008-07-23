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
#include "gsvhelper.h"

#include <string.h>
#include <glib.h>

/* gvariant-core.c */
gboolean        g_variant_has_signature         (GVariant         *variant,
                                                 GSVHelper        *signature);
GVariant       *g_variant_new_tree              (GSVHelper        *helper,
                                                 GVariant        **children,
                                                 gsize             n_children,
                                                 gboolean          trusted);
GVariant       *g_variant_new_small             (GSignature        signature,
                                                 gpointer          data,
                                                 gsize             size);
void            g_variant_get_small             (GVariant         *value,
                                                 gpointer          data,
                                                 gsize             size);
void            g_variant_ensure_native_endian  (GVariant         *value);

void            g_variant_assert_invariant      (GVariant         *value);
gboolean        g_variant_is_normalised         (GVariant         *value);

#endif /* _gvariant_private_h_ */
