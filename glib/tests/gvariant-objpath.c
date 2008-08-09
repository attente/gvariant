#include <glib.h>
#include <glib/gvariant.h>

#define TESTS              1024
#define MAX_ERRORS            2
#define MAX_ELEMENTS         20
#define MAX_ELEMENT_LENGTH   10
#define ERROR_PROBABILITY   0.1

static void
random_path (GString  *path,
             gint      errors)
{
  gint rules[6] = { 0 };
  gint elements = g_test_rand_int_range (0, MAX_ELEMENTS + 1);
  gboolean invalid = errors;
  gboolean errored = FALSE;

  while (errors--)
    rules[g_test_rand_int_range (1 + rules[1], 6 - rules[5])]++;

  if (!elements && invalid && g_test_rand_bit ())
    return;

  if (!rules[1])
    g_string_append_c (path, '/');
  else
    {
      if (rules[2] && g_test_rand_bit ())
        {
          gchar c;

          do c = (gchar) g_test_rand_int_range (' ', '~' + 1);
          while (g_ascii_isalnum (c) ||
                 c == '_' || c == '/');

          g_string_append_c (path, c);
          rules[2]--;
        }
      else
        {
          gchar c;

          do c = (gchar) g_test_rand_int_range ('0', 'z' + 1);
          while (!g_ascii_isalnum (c) && c != '_');

          g_string_append_c (path, c);
        }

      errored = TRUE;
    }

  while (elements--)
    {
      gint length = g_test_rand_int_range (1, MAX_ELEMENT_LENGTH + 1);

      if (rules[3] && g_test_rand_bit ())
        {
          rules[3]--;
          continue;
        }

      if (rules[4] &&
          path->len &&
          path->str[path->len - 1] == '/' &&
          g_test_rand_bit ())
        {
          g_string_append_c (path, '/');
          errored = TRUE;
          rules[4]--;
          continue;
        }

      while (length--)
        {
          gchar c;

          if (rules[2] && g_test_rand_double_range (0, 1) < ERROR_PROBABILITY)
            {
              do c = (gchar) g_test_rand_int_range (' ', '~' + 1);
              while (g_ascii_isalnum (c) ||
                     c == '_' || c == '/');

              g_string_append_c (path, c);
              errored = TRUE;
              rules[2]--;
              continue;
            }

          do c = (gchar) g_test_rand_int_range ('0', 'z' + 1);
          while (!g_ascii_isalnum (c) && c != '_');
          g_string_append_c (path, c);
        }

      if (elements)
        g_string_append_c (path, '/');
    }

  if (rules[5] && path->len)
    {
      g_string_append_c (path, '/');
      errored = TRUE;
    }

  if (invalid && !errored)
    g_string_append_c (path, '^');
}

static void
test (void)
{
  GString *path;
  int      i;

  for (i = 0; i < TESTS; i++)
    {
      path = g_string_new ("");

      if (g_test_rand_bit ())
        {
          random_path (path, 0);
#ifdef VERBOSE
          g_printf ("  VALID: %s\n", path->str);
#endif
          g_assert (g_variant_is_object_path (path->str));
        }
      else
        {
          random_path (path, g_test_rand_int_range (1, MAX_ERRORS + 1));
#ifdef VERBOSE
          g_printf ("INVALID: %s\n", path->str);
#endif
          g_assert (!g_variant_is_object_path (path->str));
        }

      g_string_free (path, TRUE);
    }
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/gvariant/objpath/0", test);
  return g_test_run ();
}
