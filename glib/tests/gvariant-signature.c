#include <glib.h>
#include <glib/gvariant.h>
#include <string.h>

#define VERBOSE

#define TESTS      1024
#define MAX_LENGTH 10
#define MAX_DEPTH  10
#define MAX_ERRORS 2

#define MAX_DICT_LENGTH                  4
#define MAX_STRUCT_LENGTH                10
#define PROBABILITY_OF_DICT              0.3
#define PROBABILITY_OF_NOTHING           0.1
#define PROBABILITY_OF_NON_ASCII         0.0
#define PROBABILITY_OF_BASIC_TYPE        0.3
#define PROBABILITY_OF_SPONTANEOUS_ERROR 0.1
/* change these only when protocol changes */
#define NUMBER_OF_BASIC_TYPES            10
#define NUMBER_OF_CONTAINER_TYPES        4

static void
random_type_no_errors (GString *sig,
                       int      depth)
{
  if (!depth || g_test_rand_double_range (0, 1) < PROBABILITY_OF_BASIC_TYPE)
    {
      const gchar *c = "ybnqiuxtds";
      int i = g_test_rand_int_range (0, NUMBER_OF_BASIC_TYPES);
      g_string_append_c (sig, c[i]);
    }
  else
    {
      switch (g_test_rand_int_range (0, NUMBER_OF_CONTAINER_TYPES))
        {
          case 0:
            g_string_append_c (sig, 'a');

            if (g_test_rand_double_range (0, 1) < PROBABILITY_OF_DICT)
              {
                g_string_append_c (sig, '{');
                random_type_no_errors (sig, 0);
                random_type_no_errors (sig, depth - 1);
                g_string_append_c (sig, '}');
              }
            else
              random_type_no_errors (sig, depth - 1);

            break;

          case 1:
            g_string_append_c (sig, '(');

            if (g_test_rand_double_range (0, 1) >= PROBABILITY_OF_NOTHING)
              {
                int size = g_test_rand_int_range (1, MAX_STRUCT_LENGTH + 1);

                while (size--)
                  random_type_no_errors (sig, depth - 1);
              }

            g_string_append_c (sig, ')');
            break;

          case 2:
            g_string_append_c (sig, 'v');
            break;

          case 3:
            g_string_append_c (sig, 'm');
            random_type_no_errors (sig, depth - 1);
            break;
        }
    }
}

static void
random_type (GString *sig,
             int      max_depth,
             int      errors,
             int      in_array)
{
  int rule[7] = { 0 };
  int start = sig->len;
  int i;

  if (!errors)
    {
      random_type_no_errors (sig, max_depth);
      return;
    }

  if (!max_depth || (errors == 1 && g_test_rand_bit ()))
    {
      gchar c;

      if (g_test_rand_double_range (0, 1) < PROBABILITY_OF_NON_ASCII)
        {
          do c = g_test_rand_int_range (1, 256);
          while (strchr ("ybnqiuxtdsoga()v{}m", c) != NULL);
        }
      else
        {
          do c = g_test_rand_int_range (' ', '~' + 1);
          while (strchr ("ybnqiuxtdsoga()v{}m", c) != NULL);
        }

      g_string_append_c (sig, c);
      return;
    }

  for (i = 0; i < errors; i++)
    rule[g_test_rand_int_range (0, 7)]++;

  if (rule[5] && !in_array && g_test_rand_bit ())
    {
      errors--;
      rule[5]--;

      g_string_append_c (sig, '{');

      if (!rule[4] || g_test_rand_bit ())
        {
          if (!rule[6] || g_test_rand_bit ())
            random_type (sig, 0, g_test_rand_double_range (0, 1) <
                                 PROBABILITY_OF_SPONTANEOUS_ERROR, FALSE);
          else
            random_type (sig, max_depth - 1,
                         g_test_rand_double_range (0, 1) <
                         PROBABILITY_OF_SPONTANEOUS_ERROR, FALSE);

          random_type (sig, max_depth - 1,
                       g_test_rand_double_range (0, 1) <
                       PROBABILITY_OF_SPONTANEOUS_ERROR, FALSE);
        }
      else
        {
          int length = g_test_rand_int_range (0, MAX_DICT_LENGTH);
          int j;

          if (length >= 2)
            length++;

          for (j = 0; j < length; j++)
            {
              if (!j && (!rule[6] || g_test_rand_bit ()))
                random_type (sig, 0, g_test_rand_double_range (0, 1) <
                                     PROBABILITY_OF_SPONTANEOUS_ERROR, FALSE);

              random_type (sig, max_depth - 1,
                           g_test_rand_double_range (0, 1) <
                           PROBABILITY_OF_SPONTANEOUS_ERROR, FALSE);
            }
        }

      g_string_append_c (sig, '}');
    }
  else
    {
      /* array, maybe or struct at root */
      switch (g_test_rand_int_range (0, 3))
        {
          case 0:
            if (rule[1] && g_test_rand_bit ())
              {
                errors--;
                rule[1]--;

                if (!rule[3] || g_test_rand_bit ())
                  g_string_append_c (sig, '(');
                else
                  rule[3]--;

                g_string_append_c (sig, 'a');
                g_string_append_c (sig, ')');
                return;
              }

            if (rule[2] && g_test_rand_bit ())
              {
                g_string_append_c (sig, '(');
                rule[2]--;
                errors--;
              }

            g_string_append_c (sig, 'a');
            random_type (sig, max_depth - 1, errors, TRUE);
            break;
          case 1:
            if (rule[1] && g_test_rand_bit ())
              {
                errors--;
                rule[1]--;

                if (!rule[3] || g_test_rand_bit ())
                  g_string_append_c (sig, '(');
                else
                  rule[3]--;

                g_string_append_c (sig, 'm');
                g_string_append_c (sig, ')');
                return;
              }

            if (rule[2] && g_test_rand_bit ())
              {
                g_string_append_c (sig, '(');
                rule[2]--;
                errors--;
              }

            g_string_append_c (sig, 'm');
            random_type (sig, max_depth - 1, errors, FALSE);
            break;
          case 2:
            {
              int length = g_test_rand_int_range (0, MAX_STRUCT_LENGTH + 1);
              int left = !rule[3] || g_test_rand_bit ();
              /* trust me, right should be TRUE */
              int right = !rule[2] || g_test_rand_bit () || TRUE;
              int j;

              if (!left && !right)
                {
                  if (g_test_rand_bit ())
                    left = TRUE;
                  else
                    right = TRUE;
                }

              if (left && right && !length)
                length++;

              if (left)
                g_string_append_c (sig, '(');
              else
                rule[3]--;

              for (j = 0; j < length; j++)
                random_type (sig, max_depth - 1, errors, FALSE);

              if (right)
                g_string_append_c (sig, ')');
              else
                rule[2]--;
            }
            break;
        }
    }
}

static void
test (void)
{
  GString *sig;
  int      length;
  int      max_depth;
  int      i;

  for (i = 0; i < TESTS; i++)
    {
      sig = g_string_new ("");
      length = g_test_rand_int_range (0, MAX_LENGTH + 1);
      max_depth = g_test_rand_int_range (0, MAX_DEPTH + 1);

      if (g_test_rand_bit ())
        {
          while (length--)
            random_type (sig, max_depth, 0, FALSE);

#ifdef VERBOSE
          g_printf ("  VALID: %s\n", sig->str);
#endif

          g_assert (g_variant_is_signature (sig->str));
        }
      else
        {
          int errcount = g_test_rand_int_range (1, MAX_ERRORS + 1);
          int errors[MAX_LENGTH] = { 0 };

          if (!length)
            length++;

          while (errcount--)
            errors[g_test_rand_int_range (0, length)]++;

          while (length--)
            random_type (sig, max_depth, errors[length], FALSE);

#ifdef VERBOSE
          g_printf ("INVALID: %s\n", sig->str);
#endif

          g_assert (!g_variant_is_signature (sig->str));
        }

      g_string_free (sig, TRUE);
    }
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/gvariant/signature/0", test);
  return g_test_run ();
}
