#include <glib/gvariant-loadstore.h>
#include <glib.h>

static void
test_byteswap (void)
{
  const guchar data[] = {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    0, 0, 0, 0, 0, 0, 0, 8,
    0, 0, 0, 1,
    0, 2,
    1,
    0,
#else
    8, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0,
    2, 0,
    1,
    0,
#endif
    '\0', '(', 't', 'u', 'q', 'y', 'b', ')'
  };
  GVariant *value;
  GString *string;

  value = g_variant_load (NULL, data, sizeof data, G_VARIANT_BYTESWAP_LAZY);
  string = g_variant_markup_print (value, NULL, FALSE, 0, 0);
  g_variant_unref (value);

  g_assert_cmpstr (string->str, ==, "<struct><uint64>8</uint64><uint32>1</uint32><uint16>2</uint16><byte>0x01</byte><false/></struct>");
  g_string_free (string, TRUE);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/gvariant/endian/0", test_byteswap);
  return g_test_run ();
}
