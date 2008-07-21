#include <glib/gvariant.h>
#include <glib.h>

#define TESTS           99
#define MAX_DEPTH       10
#define MAX_ARRAY_SIZE  50
#define MAX_STRUCT_SIZE 20

static gchar *
random_string (void)
{
  gchar str[4096];
  int   size;
  int   i;

  size  = 1 << ((guint32) g_test_rand_int () % 12);
  size += (guint32) g_test_rand_int () % size - 1;

  for (i = 0; i < size; i++)
    str[i] = (gchar) (rand () % ('~' - ' ' + 1) + ' ');

  return g_markup_escape_text (str, size);
}

static void
random_recurse (GString *markup,
                int      depth)
{
  depth = 0;

  if (depth == 0 || g_test_rand_bit ())
    {
      switch ((guint32) g_test_rand_int () % 10)
        {
          case 0:
            g_string_append_printf (markup,
                                    "<byte>0x%02x</byte>",
                                    (guchar) g_test_rand_int ());
            break;

          case 1:
            g_string_append_printf (markup,
                                    "<%s/>",
                                    g_test_rand_bit () ? "true" : "false");
            break;

          case 2:
            g_string_append_printf (markup,
                                    "<int16>%d</int16>",
                                    (gint16) g_test_rand_int ());
            break;

          case 3:
            g_string_append_printf (markup,
                                    "<uint16>%u</uint16>",
                                    (guint16) g_test_rand_int ());
            break;

          case 4:
            g_string_append_printf (markup,
                                    "<int32>%d</int32>",
                                    g_test_rand_int ());
            break;

          case 5:
            g_string_append_printf (markup,
                                    "<uint32>%u</uint32>",
                                    (guint32) g_test_rand_int ());
            break;

          case 6:
            g_string_append_printf (markup,
                                    "<int64>%d</int64>",
                                    g_test_rand_int ());
            break;

          case 7:
            g_string_append_printf (markup,
                                    "<uint64>%u</uint64>",
                                    (guint32) g_test_rand_int ());
            break;

          case 8:
            g_string_append_printf (markup,
                                    "<double>0.000000</double>");
            break;

          case 9:
            {
              gchar *escaped = random_string ();

              g_string_append_printf (markup,
                                      "<string>%s</string>",
                                      escaped);

              g_free (escaped);
            }
            break;
        }
    }
  else
    {
      switch ((guint32) g_test_rand_int () % 5)
        {
          case 0:
            break;
          case 1:
            break;
          case 2:
            break;
          case 3:
            break;
          case 4:
            break;
        }
    }
}

static GString *
random_markup (void)
{
  GString *markup = g_string_new ("");
  int      depth  = (guint32) g_test_rand_int () % MAX_DEPTH;

  random_recurse (markup, depth);

  return markup;
}

static void
test (void)
{
  GError   *error = NULL;
  GString  *markup1;
  GString  *markup2;
  GVariant *variant;
  int       i;

  for (i = 0; i < TESTS; i++)
    {
      markup1 = random_markup ();
      variant = g_variant_markup_parse (markup1->str, NULL, &error);

      if (variant == NULL)
        {
          g_assert (error != NULL);
          g_error (error->message);
        }
      else
        g_assert (error == NULL);

      markup2 = g_variant_markup_print (variant, NULL, FALSE, 0, 0);
      g_assert_cmpstr (markup1->str, ==, markup2->str);
      g_string_free (markup1, TRUE);
      g_string_free (markup2, TRUE);
      g_variant_unref (variant);
    }
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/gvariant/random/0", test);
  return g_test_run ();
}
