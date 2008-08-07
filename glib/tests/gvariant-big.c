#include <glib/gvariant.h>
#include <glib/gtestutils.h>
#include <glib/grand.h>

gdouble
ieee754ify (gdouble floating)
{
  return floating;
}

static const gchar *
random_string (GRand *rand)
{
  static char string[512];
  int i = 0;

  do
    string[i] = g_rand_int_range (rand, 0, 128);
  while (string[i++] != '\0' && i < 512);

  string[511] = '\0';

  return string;
}

static void
verify2 (GVariant *value,
         GRand    *rand)
{
  gsize length = g_rand_int_range (rand, 1, 100000);
  const gchar *possible = "ybquds";
  const GVariantType *type;
  GVariantTypeClass class;
  GVariantIter iter;
  GVariant *child;
  gsize actual;

  type = (const GVariantType *) (possible + g_rand_int_range (rand, 0, 6));
  class = g_variant_type_get_class (type);

  actual = g_variant_iter_init (&iter, value);
  g_assert_cmpint (actual, ==, length);

  actual = 0;
  while ((child = g_variant_iter_next (&iter)))
    {
      switch (class)
        {
          case G_VARIANT_TYPE_CLASS_BOOLEAN:
            g_assert_cmpint (g_variant_get_boolean (child), ==,
                             g_rand_int_range (rand, 0, 1));
            break;

          case G_VARIANT_TYPE_CLASS_BYTE:
            g_assert_cmpint (g_variant_get_byte (child), ==,
                             g_rand_int_range (rand, 0, 256));
            break;

          case G_VARIANT_TYPE_CLASS_UINT16:
            g_assert_cmpint (g_variant_get_uint16 (child), ==,
                             g_rand_int_range (rand, 0, 65536));
            break;

          case G_VARIANT_TYPE_CLASS_UINT32:
            g_assert_cmpint (g_variant_get_uint32 (child), ==,
                             g_rand_int (rand));
            break;

          case G_VARIANT_TYPE_CLASS_DOUBLE:
            g_assert_cmpfloat (g_variant_get_double (child), ==,
                               ieee754ify (g_rand_double (rand)));
            break;

          case G_VARIANT_TYPE_CLASS_STRING:
            g_assert_cmpstr (g_variant_get_string (child, NULL), ==,
                             random_string (rand));
            break;
            
          default:
            g_assert_not_reached ();
        }
      actual++;
    }

  g_assert_cmpint (actual, ==, length);
  g_variant_unref (value);
}


static GVariant *
generate2 (GRand *rand)
{
  gsize length = g_rand_int_range (rand, 1, 100000);
  const gchar *possible = "ybquds";
  const GVariantType *type;
  GVariantTypeClass class;
  GVariantBuilder *builder;
  gsize i;

  type = (const GVariantType *) (possible + g_rand_int_range (rand, 0, 6));
  class = g_variant_type_get_class (type);

  builder = g_variant_builder_new (G_VARIANT_TYPE_CLASS_ARRAY, NULL);
  for (i = 0; i < length; i++)
    {
      switch (class)
        {
          case G_VARIANT_TYPE_CLASS_BOOLEAN:
            g_variant_builder_add (builder, "b",
                                   g_rand_int_range (rand, 0, 1));
            break;

          case G_VARIANT_TYPE_CLASS_BYTE:
            g_variant_builder_add (builder, "y",
                                   g_rand_int_range (rand, 0, 256));
            break;

          case G_VARIANT_TYPE_CLASS_UINT16:
            g_variant_builder_add (builder, "q",
                                   g_rand_int_range (rand, 0, 65536));
            break;

          case G_VARIANT_TYPE_CLASS_UINT32:
            g_variant_builder_add (builder, "u", g_rand_int (rand));
            break;

          case G_VARIANT_TYPE_CLASS_DOUBLE:
            g_variant_builder_add (builder, "d", g_rand_double (rand));
            break;

          case G_VARIANT_TYPE_CLASS_STRING:
            g_variant_builder_add (builder, "s", random_string (rand));
            break;
            
          default:
            g_assert_not_reached ();
        }
    }

  return g_variant_new_variant (g_variant_builder_end (builder));
}

static GVariant *
generate (GRand *rand)
{
  gsize length = g_rand_int_range (rand, 0, 20);
  GVariantBuilder *builder;
  gsize i;

  builder = g_variant_builder_new (G_VARIANT_TYPE_CLASS_ARRAY,
                                   G_VARIANT_TYPE ("av"));

  /* length will fall into 0, 0..255, 256..65535, and >65536 ranges */
  for (i = 0; i < length; i++)
    g_variant_builder_add_value (builder, generate2 (rand));

  return g_variant_builder_end (builder);
}

static void
verify (GVariant *value,
        GRand    *rand)
{
  gsize length = g_rand_int_range (rand, 0, 20);
  GVariantIter iter;
  GVariant *child;
  gsize actual;

  actual = g_variant_iter_init (&iter, value);
  g_assert_cmpint (actual, ==, length);
  actual = 0;

  while ((child = g_variant_iter_next (&iter)))
    {
      verify2 (g_variant_get_variant (child), rand);
      actual++;
    }

  g_assert_cmpint (actual, ==, length);
}

static void
test (void)
{
  GRand *one, *two;
  GVariant *value;
  guint32 seed;

  seed = g_test_rand_int ();
  one = g_rand_new_with_seed (seed);
  two = g_rand_new_with_seed (seed);

  g_assert_cmpfloat (ieee754ify (g_rand_double (one)), ==, g_variant_get_double (g_variant_new_double (g_rand_double (two))));

  value = generate (one);
  g_rand_free (one);

  g_variant_flatten (value);

  verify (value, two);
  g_rand_free (two);

  g_variant_unref (value);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/gvariant/big", test);

  return g_test_run ();
}
