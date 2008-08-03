#include <glib/gvariant.h>
#include <string.h>
#include <glib.h>

#include <stdio.h>

int
main (int argc, char **argv)
{
  GMarkupParseContext *context;
  GError *error = NULL;
  FILE *file = stdin;
  char buffer[2048];
  GString *output;
  GVariant *value;
  gsize size;
  gint i = 1;

  context = g_variant_markup_parse_context_new (G_MARKUP_PREFIX_ERROR_POSITION,
                                                NULL);

  do
    {
      if (i < argc)
        {
          if (!strcmp (argv[i], "-"))
            file = stdin;
          else
            file = fopen (argv[i], "r");

          if (file == NULL)
            g_error ("error opening file '%s'", argv[i]);
        }

        while (!feof (file) &&
               (size = fread (buffer, 1, sizeof buffer / 1, file)) > 0)
          if (!g_markup_parse_context_parse (context, buffer, size, &error))
            g_error ("%s", error->message);

        if (ferror (file))
          g_error ("file error on %s", (file == stdin) ? "stdin" : argv[i]);

        if (file != stdin)
          fclose (file);
    }
  while (++i < argc);

  if (!(value = g_variant_markup_parse_context_end (context, &error)))
    g_error ("value error: %s", error->message);

  g_variant_flatten (value);

  output = g_variant_markup_print (value, NULL, TRUE, 0, 2);
  g_variant_unref (value);

  g_print ("%s", output->str);
  g_string_free (output, TRUE);

  return 0;
}
