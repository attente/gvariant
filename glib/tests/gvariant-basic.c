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
test_int64 (void)
{
  int i;

  static struct
  {
    const char *type;
    gint64 value;
  } values[] = {
    { (const char *) G_SIGNATURE_INT64, G_GINT64_CONSTANT(0xffffffffffffffff) },
    { (const char *) G_SIGNATURE_INT64, G_GINT64_CONSTANT(0x7fffffffffffffff) },
    { (const char *) G_SIGNATURE_INT64, G_GINT64_CONSTANT(0x0000000000000000) },
    { (const char *) G_SIGNATURE_INT64, G_GINT64_CONSTANT(4242424242424242) },
    { (const char *) G_SIGNATURE_INT64, G_GINT64_CONSTANT(0x0101010101010101) },
    { (const char *) G_SIGNATURE_INT64, G_GINT64_CONSTANT(0x1010101010101010) },
  };

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      GVariant *variant = g_variant_new (values[i].type, values[i].value);
      gint64 test;
      g_variant_get (variant, values[i].type, &test);

      g_assert_cmpint (test, ==, values[i].value);

      g_variant_unref (variant);
    }

  return;
}


static void
test_uint64 (void)
{
  int i;

  static struct
  {
    const char *type;
    guint64 value;
  } values[] = {
    { (const char *) G_SIGNATURE_UINT64, G_GUINT64_CONSTANT(0xffffffffffffffff) },
    { (const char *) G_SIGNATURE_UINT64, G_GUINT64_CONSTANT(0x7fffffffffffffff) },
    { (const char *) G_SIGNATURE_UINT64, G_GUINT64_CONSTANT(0x0000000000000000) },
    { (const char *) G_SIGNATURE_UINT64, G_GUINT64_CONSTANT(4242424242424242) },
    { (const char *) G_SIGNATURE_UINT64, G_GUINT64_CONSTANT(0x0101010101010101) },
    { (const char *) G_SIGNATURE_UINT64, G_GUINT64_CONSTANT(0x1010101010101010) },  };

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      GVariant *variant = g_variant_new (values[i].type, values[i].value);
      guint64 test;
      g_variant_get (variant, values[i].type, &test);

      g_assert_cmpint (test, ==, values[i].value);

      g_variant_unref (variant);
    }

  return;
}

static void
test_double (void)
{
  int i;

  static struct
  {
    const char *type;
    gdouble value;
  } values[] = {
    { (const char *) G_SIGNATURE_DOUBLE, 0.123 },
    { (const char *) G_SIGNATURE_DOUBLE, 123 },
    { (const char *) G_SIGNATURE_DOUBLE, 0.1234 },
    { (const char *) G_SIGNATURE_DOUBLE, 1.5567 },
    { (const char *) G_SIGNATURE_DOUBLE, 1.5563 },
  };

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      GVariant *variant = g_variant_new (values[i].type, values[i].value);
      gdouble test;
      g_variant_get (variant, values[i].type, &test);

      g_assert_cmpint (test, ==, values[i].value);

      g_variant_unref (variant);
    }

  return;
}

static void
test_int32 (void)
{
  int i;

  static struct
  {
    const char *type;
    gint32 value;
  } values[] = {
    { (const char *) G_SIGNATURE_INT32, 0xffffffff },
    { (const char *) G_SIGNATURE_INT32, 0x7fffffff },
    { (const char *) G_SIGNATURE_INT32, 0x00000000 },
    { (const char *) G_SIGNATURE_INT32, 42424242 },
    { (const char *) G_SIGNATURE_INT32, 0x01010101 },
    { (const char *) G_SIGNATURE_INT32, 0x10101010 },
  };

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      GVariant *variant = g_variant_new (values[i].type, values[i].value);
      gint32 test;
      g_variant_get (variant, values[i].type, &test);

      g_assert_cmpint (test, ==, values[i].value);

      g_variant_unref (variant);
    }

  return;
}


static void
test_uint32 (void)
{
  int i;

  static struct
  {
    const char *type;
    guint32 value;
  } values[] = {
    { (const char *) G_SIGNATURE_UINT32, 0xffffffff },
    { (const char *) G_SIGNATURE_UINT32, 0x7fffffff },
    { (const char *) G_SIGNATURE_UINT32, 0x00000000 },
    { (const char *) G_SIGNATURE_UINT32, 42424242 },
    { (const char *) G_SIGNATURE_UINT32, 0x01010101 },
    { (const char *) G_SIGNATURE_UINT32, 0x10101010 },
  };

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      GVariant *variant = g_variant_new (values[i].type, values[i].value);
      guint32 test;
      g_variant_get (variant, values[i].type, &test);

      g_assert_cmpint (test, ==, values[i].value);

      g_variant_unref (variant);
    }

  return;
}

static void
test_int16 (void)
{
  int i;
  gint16 b;

  for (b = 0xffff; b < 0x7fff; b++)
    {
      GVariant *variant = g_variant_new ((const char *) G_SIGNATURE_INT16, b);
      gint16 test;
      g_variant_get (variant, (const char *) G_SIGNATURE_INT16, &test);

      g_assert_cmpint (test, ==, b);

      g_variant_unref (variant);
    }

  return;
}


static void
test_uint16 (void)
{
  int i;
  guint16 b;

  for (b = 0xffff; b < 0x7fff; b++)
    {
      GVariant *variant = g_variant_new ((const char *) G_SIGNATURE_UINT16, b);
      guint16 test;
      g_variant_get (variant, (const char *) G_SIGNATURE_UINT16, &test);

      g_assert_cmpint (test, ==, b);

      g_variant_unref (variant);
    }

  return;
}

static void
test_byte (void)
{
  int i;
  guint8 b;

  for (b = 0; b < 255; b++)
    {
      GVariant *variant = g_variant_new ((const char *) G_SIGNATURE_BYTE, b);
      guint8 test;
      g_variant_get (variant, (const char *) G_SIGNATURE_BYTE, &test);

      g_assert_cmpint (test, ==, b);

      g_variant_unref (variant);
    }

  return;
}


static void
test_string (void)
{
  int i;

  static struct
  {
    const char *type;
    const char *value;
  } values[] = {
    { (const char *) G_SIGNATURE_STRING, "" },
    { (const char *) G_SIGNATURE_STRING, "the anti string" },
    { (const char *) G_SIGNATURE_STRING, "<xml attr=\"test\"><![CDATA[ abdbdb\n\n%&*@&#\n]]></xml>" },
    { (const char *) G_SIGNATURE_STRING, "I do, I in fact , I insist" },
  };

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      GVariant *variant = g_variant_new (values[i].type, values[i].value);
      char* test;
      g_variant_get (variant, values[i].type, &test);

      g_assert_cmpstr (test, ==, values[i].value);
      g_free (test);

      g_variant_unref (variant);
    }

  return;
}

static void
test_boolean (void)
{
  int i;

  static struct
  {
    const char *type;
    gboolean value;
  } values[] = {
    { (const char *) G_SIGNATURE_BOOLEAN, TRUE },
    { (const char *) G_SIGNATURE_BOOLEAN, FALSE },
  };

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      GVariant *variant = g_variant_new (values[i].type, values[i].value);
      gboolean test;
      g_variant_get (variant, values[i].type, &test);

      g_assert_cmpint (test, ==, values[i].value);

      g_variant_unref (variant);
    }

  return;
}

int
main (int argc, char **argv)
{
  guint limit = 1000;
  int i;

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/variant/basic/boolean", test_boolean);
  g_test_add_func ("/variant/basic/string", test_string);
  g_test_add_func ("/variant/basic/byte", test_byte);
  g_test_add_func ("/variant/basic/uint16", test_uint16);
  g_test_add_func ("/variant/basic/int16", test_int16);
  g_test_add_func ("/variant/basic/int32", test_int32);
  g_test_add_func ("/variant/basic/uint32", test_int32);
  g_test_add_func ("/variant/basic/double", test_double);
  g_test_add_func ("/variant/basic/int64", test_int64);
  g_test_add_func ("/variant/basic/uint64", test_uint64);

  return g_test_run ();
}

