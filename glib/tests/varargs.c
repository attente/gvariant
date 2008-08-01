#include <glib/gvariant.h>
#include <glib/ghash.h>

int
main (void)
{
  GVariant *value;
  GString *string;

  value = g_variant_new ("m(ii)", TRUE, 800, 600);
  string = g_variant_markup_print (value, NULL, TRUE, 0, 2);

  g_print ("%s", string->str);

  return 0;
}
