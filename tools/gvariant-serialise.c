#include <glib/gvariant-loadstore.h>
#include <string.h>
#include <glib.h>

#include <stdio.h>

int
main (int argc, char **argv)
{
  static const GMarkupParser null_parser;
  GMarkupParseContext *context;
  GError *error = NULL;
  FILE *file = stdin;
  gconstpointer data;
  char buffer[2048];
  GVariant *value;
  gboolean raw;
  gsize size;
  gint i = 1;

  context = g_markup_parse_context_new (&null_parser,
                                        G_MARKUP_PREFIX_ERROR_POSITION,
                                        NULL, NULL);
  g_variant_markup_parser_start_parse (context, NULL);

  raw = FALSE;

  if (i < argc && strcmp (argv[i], "-b") == 0)
    {
      raw = TRUE;
      i++;
    }

  do
    {
      if (i < argc)
        {
          if (!strcmp (argv[i], "-"))
            file = stdin;
          else
            file = fopen (argv[i], "r");

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

  if (!g_markup_parse_context_end_parse (context, &error))
    g_error ("error at eof: %s", error->message);

  if (!(value = g_variant_markup_parser_end_parse (context, &error)))
    g_error ("value error: %s", error->message);

  g_markup_parse_context_free (context);

  data = g_variant_get_data (value);
  size = g_variant_get_size (value);

  if (raw)
    file = stdout;
  else
    file = popen ("hexdump -C", "w");

  if (file == NULL)
    g_error ("failed to open output");

  fwrite (data, 1, size, file);

  if (file != stdout)
    fclose (file);

  g_variant_unref (value);

  return 0;
}
