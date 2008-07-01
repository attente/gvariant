/*
 * Copyright © 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 * 
 * See the included COPYING file for more information.
 */

#ifndef _gsvhelper_h_
#define _gsvhelper_h_

#include "gsignature.h"

typedef struct OPAQUE_TYPE_GSVHelper GSVHelper;

/* gsignature-helper.c */
typedef struct
{
  GSVHelper *helper;

  gsize index, plus;
  gint8 and, or;

  gssize size;
} MemberInfo;

#define STRUCT_MEMBER_LAST         (-1)
#define STRUCT_MEMBER_VARIABLE     (-2)

gboolean        g_svhelper_equal              (GSVHelper    *one,
                                               GSVHelper    *two);
GSignature      g_svhelper_signature          (GSVHelper    *helper);
gsize           g_svhelper_n_members          (GSVHelper    *helper);
GSignatureType  g_svhelper_type               (GSVHelper    *helper);
GSVHelper      *g_svhelper_element            (GSVHelper    *helper);
void            g_svhelper_element_info       (GSVHelper    *helper,
                                               guint        *alignment,
                                               gssize       *size);
void            g_svhelper_info               (GSVHelper    *signature,
                                               guint        *alignment,
                                               gssize       *size);
gboolean        g_svhelper_member_info        (GSVHelper    *signature,
                                               gsize         index,
                                               MemberInfo   *info);
GSVHelper      *g_svhelper_scan               (const guchar *data,
                                               gsize        *size);

GSVHelper      *g_svhelper_get                (GSignature    signature);
GSVHelper      *g_svhelper_ref                (GSVHelper    *helper);
void            g_svhelper_unref              (GSVHelper    *helper);

#endif /* _gsvhelper_h_ */
