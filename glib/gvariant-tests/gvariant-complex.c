/*
 * Copyright © 2008 Philip Van Hoof, Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.   
 * 
 * See the included COPYING file for more information.
 */

#include <glib/gvariant.h>
#include <glib.h>

static void
test_s_ii_v (void)
{
  GVariant *t, *variant;
  gchar *s;
  gint i1,i2;
  gboolean mybool;

  variant = g_variant_new ("(s(ii)v)",
                           "Hello World", 800, 600,
                           g_variant_new_boolean (TRUE));

  g_variant_get (variant, "(s(ii)v)", &s, &i1, &i2, &t);

  g_variant_get (t, "b", &mybool);
  g_assert_cmpint (mybool, ==, TRUE);
  g_variant_unref (t);

  g_assert_cmpstr (s, ==, "Hello World");
  g_assert_cmpint (i1, ==, 800);
  g_assert_cmpint (i2, ==, 600);
  g_free (s);

  g_variant_unref (variant);
}

static void
test_a_si (void)
{
  gchar *szero = NULL, *sone = NULL, *stwo = NULL, *sextra = NULL;
  gint zero, one, two, extra, len;
  GVariantIter iter;
  GVariant *variant;
  GVariant *array;

  variant = g_variant_new ("(a{si}s)",
                           3,
                             "zero", 0,
                             "one",  1,
                             "two",  2,
                           "");

  array = g_variant_get_child (variant, 0);
  len = g_variant_iter_init (&iter, array);
  g_variant_unref (variant);

  g_assert_cmpint (len, ==, 3);

  g_variant_iterate (&iter, "{si}", &szero, &zero);
  g_variant_iterate (&iter, "{si}", &sone, &one);
  g_variant_iterate (&iter, "{si}", &stwo, &two);

  g_assert (g_variant_iterate (&iter, "si", &sextra, &extra) == FALSE);

  g_assert_cmpstr (szero, ==, "zero");
  g_assert_cmpint (zero, ==, 0);

  g_assert_cmpstr (sone, ==, "one");
  g_assert_cmpint (one, ==, 1);

  g_assert_cmpstr (stwo, ==, "two");
  g_assert_cmpint (two, ==, 2);

  g_variant_unref (array);
  g_free (szero);
  g_free (sone);
  g_free (stwo);
}

static void 
test_s_vvvv_v (void)
{
  GVariant *variant;
  guint16 myuint16;
  gboolean myboolean;
  gchar *mystring, *s;
  guint8 mymaxbyte, myminbyte;
  GVariant *t1, *t2, *t3, *t4, *t5;

  variant = g_variant_new ("(s(vvvv)v)",
                           "Crack example",
                           g_variant_new_boolean (TRUE),
                           g_variant_new_string ("abc"),
                           g_variant_new_uint16 (0xfff3),
                           g_variant_new_byte (255),
			   g_variant_new_byte (0));

  g_variant_get (variant, "(s(vvvv)v)", &s, 
                 &t1, &t2, &t3, &t4, &t5);

  g_variant_get (t1, (const char *) G_SIGNATURE_BOOLEAN, &myboolean);
  g_variant_get (t2, (const char *) G_SIGNATURE_STRING, &mystring);
  g_variant_get (t3, (const char *) G_SIGNATURE_UINT16, &myuint16);
  g_variant_get (t4, (const char *) G_SIGNATURE_BYTE, &mymaxbyte);
  g_variant_get (t5, (const char *) G_SIGNATURE_BYTE, &myminbyte);

  g_assert_cmpstr (s, ==, "Crack example");
  g_assert_cmpint (myboolean, ==, TRUE);
  g_assert_cmpstr (mystring, ==, "abc");
  g_assert_cmpint (myuint16, ==, 0xfff3);
  g_assert_cmpint (mymaxbyte, ==, 255);
  g_assert_cmpint (myminbyte, ==, 0);

  g_variant_unref (variant);
  g_variant_unref (t1);
  g_variant_unref (t2);
  g_variant_unref (t3);
  g_variant_unref (t4);
  g_variant_unref (t5);
  g_free (mystring);
  g_free (s);
}

static void 
test_vvvv (void)
{
  guint16 myuint16;
  gboolean myboolean;
  gchar *mystring;
  guint8 mymaxbyte, myminbyte;
  GVariant *t1, *t2, *t3, *t4;
  GVariant *variant;
 
  variant = g_variant_new ("(vvvv)",
                           g_variant_new_boolean (TRUE),
                           g_variant_new_string ("abc"),
                           g_variant_new_uint16 (0xfff3),
                           g_variant_new_byte (255));

  g_variant_get (variant, "(vvvv)", 
                 &t1, &t2, &t3, &t4);

  g_variant_get (t1, (const char *) G_SIGNATURE_BOOLEAN, &myboolean);
  g_variant_get (t2, (const char *) G_SIGNATURE_STRING, &mystring);
  g_variant_get (t3, (const char *) G_SIGNATURE_UINT16, &myuint16);
  g_variant_get (t4, (const char *) G_SIGNATURE_BYTE, &mymaxbyte);

  g_assert_cmpint (myboolean, ==, TRUE);
  g_assert_cmpstr (mystring, ==, "abc");
  g_assert_cmpint (myuint16, ==, 0xfff3);
  g_assert_cmpint (mymaxbyte, ==, 255);

  g_variant_unref (variant);
  g_variant_unref (t1);
  g_variant_unref (t2);
  g_variant_unref (t3);
  g_variant_unref (t4);
  g_free (mystring);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/variant/complex/s(ii)v", test_s_ii_v);
  g_test_add_func ("/variant/complex/vvvv", test_vvvv);

  g_test_add_func ("/variant/complex/a{si}", test_a_si);
  g_test_add_func ("/variant/complex/s(vvvv)v", test_s_vvvv_v);

  return g_test_run ();
}
