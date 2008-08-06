/*
 * Copyright © 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#ifndef _gvarianttypeinfo_h_
#define _gvarianttypeinfo_h_

#include "gvarianttype.h"

typedef struct OPAQUE_TYPE__GVariantTypeInfo GVariantTypeInfo;

typedef struct
{
  GVariantTypeInfo *type;

  gsize i, a;
  gint8 b, c;
} GVariantMemberInfo;

#define STRUCT_MEMBER_LAST         (-1)
#define STRUCT_MEMBER_VARIABLE     (-2)

/* query */
const GVariantType             *g_variant_type_info_get_type            (GVariantTypeInfo   *typeinfo);
GVariantTypeClass               g_variant_type_info_get_type_class      (GVariantTypeInfo   *typeinfo);
const gchar                    *g_variant_type_info_get_string          (GVariantTypeInfo   *typeinfo);

void                            g_variant_type_info_query               (GVariantTypeInfo   *typeinfo,
                                                                         guint              *alignment,
                                                                         gssize             *size);

/* array */
GVariantTypeInfo               *g_variant_type_info_element             (GVariantTypeInfo   *typeinfo);
void                            g_variant_type_info_query_element       (GVariantTypeInfo   *typeinfo,
                                                                         guint              *alignment,
                                                                         gssize             *size);

/* structure */
gsize                           g_variant_type_info_n_members           (GVariantTypeInfo   *typeinfo);
const GVariantMemberInfo       *g_variant_type_info_member_info         (GVariantTypeInfo   *typeinfo,
                                                                         gsize               index);

/* new/ref/unref */
GVariantTypeInfo               *g_variant_type_info_get                 (const GVariantType *type);
GVariantTypeInfo               *g_variant_type_info_ref                 (GVariantTypeInfo   *typeinfo);
void                            g_variant_type_info_unref               (GVariantTypeInfo   *typeinfo);

#endif /* _gvarianttypeinfo_h_ */
