#include <glib.h>
#include <glib/gvariant.h>

#define TESTS                     65536
#define MAXIMUM_DEPTH             20
#define MAXIMUM_ARRAY_SIZE        20
#define MAXIMUM_STRUCT_SIZE       20
/* the probability of an empty array/struct */
#define PROBABILITY_OF_NOTHING    0.1
#define PROBABILITY_OF_BASIC_TYPE 0.3
/* log base 2 of maximum string length */
#define LOG_2_MAXIMUM_STRING_SIZE 12

/* change these only when protocol changes */
#define NUMBER_OF_BASIC_TYPES     10
#define NUMBER_OF_CONTAINER_TYPES 5

static void         random_signature             (GString *signature,
                                                  int      depth);
static gchar       *random_string                (void);
static const gchar *next_type_in_signature       (const gchar *sig);
static const gchar *random_markup_from_signature (GString     *markup,
                                                  const gchar *signature,
                                                  int          depth);
static void         random_markup                (GString *markup,
                                                  int      depth);
static void         test                         (void);

static void
random_signature (GString *signature,
                  int      depth)
{
  if (!depth || g_test_rand_double_range (0, 1) < PROBABILITY_OF_BASIC_TYPE)
    {
      const gchar *c = "ybnqiuxtds";
      int i = g_test_rand_int_range (0, NUMBER_OF_BASIC_TYPES);
      g_string_append_c (signature, c[i]);
    }
  else
    {
      switch (g_test_rand_int_range (0, NUMBER_OF_CONTAINER_TYPES))
        {
          case 0:
            g_string_append_c (signature, 'a');
            random_signature (signature, depth - 1);
            break;

          case 1:
            g_string_append_c (signature, '(');

            if (g_test_rand_double_range (0, 1) >= PROBABILITY_OF_NOTHING)
              {
                int size = g_test_rand_int_range (1, MAXIMUM_STRUCT_SIZE + 1);

                while (size--)
                  random_signature (signature, depth - 1);
              }

            g_string_append_c (signature, ')');
            break;

          case 2:
            g_string_append_c (signature, 'v');
            break;

          case 3:
            g_string_append_c (signature, '{');
            random_signature (signature, 0);
            random_signature (signature, depth - 1);
            g_string_append_c (signature, '}');
            break;

          case 4:
            g_string_append_c (signature, 'm');
            random_signature (signature, depth - 1);
            break;
        }
    }
}

static gchar *
random_string (void)
{
  gchar str[1 << LOG_2_MAXIMUM_STRING_SIZE];
  int   size;
  int   i;

  size = 1 << g_test_rand_int_range (0, LOG_2_MAXIMUM_STRING_SIZE);
  size += g_test_rand_int_range (-1, size - 1);

  for (i = 0; i < size; i++)
    str[i] = (gchar) g_test_rand_int_range (' ', '~' + 1);

  return g_markup_escape_text (str, size);
}

static const gchar *
next_type_in_signature (const gchar *sig)
{
  while (*sig == 'a' || *sig == 'm')
    sig++;

  if (*sig == '(' || *sig == '{')
    for (sig++; *sig != ')' && *sig != '}';
         sig = next_type_in_signature (sig));

  return sig + 1;
}

static const gchar *
random_markup_from_signature (GString     *markup,
                              const gchar *signature,
                              int          depth)
{
  switch (*signature)
    {
      case 'y':
        g_string_append_printf (markup,
                                "<byte>0x%02x</byte>",
                                (guint8) g_test_rand_int ());
        return signature + 1;

      case 'b':
        g_string_append_printf (markup,
                                "<%s/>",
                                g_test_rand_bit () ? "true" : "false");
        return signature + 1;

      case 'n':
        g_string_append_printf (markup,
                                "<int16>%d</int16>",
                                (gint16) g_test_rand_int ());
        return signature + 1;

      case 'q':
        g_string_append_printf (markup,
                                "<uint16>%u</uint16>",
                                (guint16) g_test_rand_int ());
        return signature + 1;

      case 'i':
        g_string_append_printf (markup,
                                "<int32>%d</int32>",
                                (gint32) g_test_rand_int ());
        return signature + 1;

      case 'u':
        g_string_append_printf (markup,
                                "<uint32>%u</uint32>",
                                (guint32) g_test_rand_int ());
        return signature + 1;

      case 'x':
        g_string_append_printf (markup,
                                "<int64>%d</int64>",
                                (gint64) g_test_rand_int ());
        return signature + 1;

      case 't':
        g_string_append_printf (markup,
                                "<uint64>%u</uint64>",
                                (guint64) g_test_rand_int ());
        return signature + 1;

      case 'd':
        g_string_append_printf (markup,
                                "<double>%lf</double>",
                                g_test_rand_double ());
        return signature + 1;

      case 's':
        {
          gchar *escaped = random_string ();

          g_string_append_printf (markup,
                                  "<string>%s</string>",
                                  escaped);

          g_free (escaped);
          return signature + 1;
        }

      case 'a':
        if (g_test_rand_double_range (0, 1) >= PROBABILITY_OF_NOTHING)
          {
            const gchar *next;
            int size;

            g_string_append (markup, "<array>");

            for (size = g_test_rand_int_range (1, MAXIMUM_ARRAY_SIZE + 1);
                 size--;
                 next = random_markup_from_signature (markup,
                                                      signature + 1,
                                                      depth - 1));

            signature = next;

            g_string_append (markup, "</array>");
          }
        else
          {
            const gchar *next = next_type_in_signature (++signature);

            g_string_append (markup, "<array signature='");
            g_string_append_len (markup, signature, next - signature);
            g_string_append (markup, "'/>");

            signature = next;
          }

        return signature;

      case '(':
        if (*++signature == ')')
          g_string_append (markup, "<triv/>");
        else
          {
            g_string_append (markup, "<struct>");

            for (;
                 *signature != ')';
                 signature = random_markup_from_signature (markup,
                                                           signature,
                                                           depth - 1));

            g_string_append (markup, "</struct>");
          }

        return signature + 1;

      case 'v':
        random_markup (markup, depth - 1);
        return signature + 1;

      case '{':
        g_string_append (markup, "<dictionary-entry>");

        for (signature++;
             *signature != '}';
             signature = random_markup_from_signature (markup,
                                                       signature,
                                                       depth - 1));

        g_string_append (markup, "</dictionary-entry>");
        return signature + 1;

      case 'm':
        if (g_test_rand_bit ())
          {
            g_string_append (markup, "<maybe>");

            signature = random_markup_from_signature (markup,
                                                      signature + 1,
                                                      depth - 1);

            g_string_append (markup, "</maybe>");
          }
        else
          {
            const gchar *next = next_type_in_signature (++signature);

            g_string_append (markup, "<nothing signature='");
            g_string_append_len (markup, signature, next - signature);
            g_string_append (markup, "'/>");

            signature = next;
          }

        return signature;
    }
}

static void
random_markup (GString *markup,
               int      depth)
{
  GString *signature = g_string_new ("");

  random_signature (signature, depth);
  random_markup_from_signature (markup, signature->str, depth);

  g_string_free (signature, TRUE);
}

static void
test (void)
{
  GError   *error = NULL;
  GString  *markup1;
  GString  *markup2;
  GVariant *variant;
  int       depth;
  int       i;

  for (i = 0; i < TESTS; i++)
    {
      markup1 = g_string_new ("");
      depth = g_test_rand_int_range (0, MAXIMUM_DEPTH);

      random_markup (markup1, depth);
      g_message ("%s", markup1->str);
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
