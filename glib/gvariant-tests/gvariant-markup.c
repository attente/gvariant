#include <glib/gvariant.h>
#include <glib/gvariant-loadstore.h>
#include <glib.h>

#define add_tests(func, basename, array) \
  G_STMT_START { \
    int __add_tests_i;                                                  \
                                                                        \
    for (__add_tests_i  = 0;                                            \
         __add_tests_i < G_N_ELEMENTS (array);                          \
         __add_tests_i++)                                               \
      {                                                                 \
        char *testname;                                                 \
                                                                        \
        testname = g_strdup_printf ("%s/%d", basename, __add_tests_i);  \
        g_test_add_data_func (testname, array[__add_tests_i], func);    \
        g_free (testname);                                              \
      }                                                                 \
  } G_STMT_END

const char *verbatim_tests[] = {
  "<array signature='ai'/>",

  "<struct>"
    "<array>"
      "<int32>1</int32>"
      "<int32>2</int32>"
      "<int32>3</int32>"
    "</array>"
    "<array>"
      "<array signature='aaai'/>"
      "<array signature='aaai'/>"
      "<array signature='aaai'/>"
    "</array>"
  "</struct>",

  "<array>"
    "<string>hello world</string>"
    "<string>how &lt;are&gt; you</string>"
    "<string> working -- i -- hope    </string>"
    "<string>%s %d %s %d</string>"
    "<string></string>"
  "</array>",

  "<array>"
    "<array>"
      "<string>foo</string>"
    "</array>"
    "<array signature='as'/>"
  "</array>",

  "<maybe>"
    "<struct>"
      "<uint32>42</uint32>"
      "<byte>0x42</byte>"
      "<int32>-1</int32>"
      "<double>37.500000</double>"
      "<int64>-35383472451088536</int64>"
      "<uint64>9446744073709551616</uint64>"
    "</struct>"
  "</maybe>",

  "<struct>"
    "<array>"
      "<true/>"
      "<false/>"
      "<true/>"
      "<true/>"
      "<false/>"
      "<false/>"
      "<true/>"
      "<true/>"
    "</array>"
    "<array>"
      "<triv/>"
      "<triv/>"
    "</array>"
  "</struct>"
};

static void
check_verbatim (gconstpointer data)
{
  const char *markup = data;
  GError *error = NULL;
  GVariant *value;
  GString *out;

  value = g_variant_markup_parse (markup, NULL, &error);

  if (value == NULL)
    {
      g_assert (error != NULL);
      g_error ("%s", error->message);
    }
  else
    g_assert (error == NULL);

  g_variant_get_data (value);

  out = g_variant_markup_print (value, NULL, FALSE, 0, 0);
  g_assert_cmpstr (markup, ==, out->str);
  g_string_free (out, TRUE);
  g_variant_unref (value);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  add_tests (check_verbatim, "/gvariant/markup/verbatim", verbatim_tests);
  return g_test_run ();
}
