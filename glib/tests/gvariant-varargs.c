#include <glib/gvariant.h>

int
main (void)
{
  GVariant *value;
  GString *string;

  value = g_variant_new ("m(msmi)", TRUE, NULL, TRUE, 1234);
  string = g_variant_markup_print (value, NULL, TRUE, 0, 2);

  g_print ("%s", string->str);

  g_string_free (string, TRUE);
  g_variant_unref (value);

  return 0;
}
