#include <glib/gvariant-loadstore.h>
#include <glib.h>
#include <stdio.h>

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
        g_test_add_data_func (testname, &array[__add_tests_i], func);   \
        g_free (testname);                                              \
      }                                                                 \
  } G_STMT_END

struct test_case
{
  const char *type;
  int size;
  const char *data;
  const char *markup;
};

#define testcase(type, binary, markup) \
  { type, sizeof binary - 1, binary, markup }

struct test_case test_cases[] = {
  testcase ("as", "foo\0bar\0se\0\x4\x8\xb",
            "<array>"
              "<string>foo</string>"
              "<string>bar</string>"
              "<string>se</string>"
            "</array>"),
  testcase ("(syus)", "str\0\xaa\0\0\0\x1\x1\x1\x1theend\0\x4",
            "<struct>"
              "<string>str</string>"
              "<byte>0xaa</byte>"
              "<uint32>16843009</uint32>"
              "<string>theend</string>"
            "</struct>"),
  testcase ("a(sss)", "hello\0world\0gvariant\0\xc\x6" /*         0x17 */
                      "this\0hopefully\0works\0\xf\x5" /* +0x17 = 0x2e */
                      "k\0thx\0bye\0\x6\x2"            /* +0x0c = 0x3a */
                      "\x17\x2e\x3a",
            "<array>"
              "<struct>"
                "<string>hello</string>"
                "<string>world</string>"
                "<string>gvariant</string>"
              "</struct>"
              "<struct>"
                "<string>this</string>"
                "<string>hopefully</string>"
                "<string>works</string>"
              "</struct>"
              "<struct>"
                "<string>k</string>"
                "<string>thx</string>"
                "<string>bye</string>"
              "</struct>"
            "</array>")

};

void
test (gconstpointer data)
{
  const struct test_case *tc = data;
  GVariant *variant;
  GString *markup;

  variant = g_variant_load (G_VARIANT_TYPE (tc->type), tc->data, tc->size, 0);
  markup = g_variant_markup_print (variant, NULL, FALSE, 0, 0);
  g_variant_unref (variant);

  g_assert_cmpstr (tc->markup, ==, markup->str);
  g_string_free (markup, TRUE);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  add_tests (test, "/gvariant/serialiser", test_cases);
  return g_test_run ();
}
