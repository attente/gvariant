#include <glib.h>

void
test (void)
{
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/gvariant/random/0", test);
  return g_test_run ();
}
