#include <glib/gvariant.h>
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
  char buffer[2048];
  GString *output;
  GVariant *value;
  gsize size;
  gint i = 1;

  context = g_markup_parse_context_new (&null_parser,
                                        G_MARKUP_PREFIX_ERROR_POSITION,
                                        NULL, NULL);
  g_variant_markup_parser_start_parse (context, NULL);

  do
    {
      if (i < argc)
        {
          g_print ("it is %s\n", argv[i]);
          if (!strcmp (argv[i], "-"))
            file = stdin;
          else
            file = fopen (argv[i], "r");

          if (file == NULL)
            g_error ("error opening file '%s'", argv[i]);
        }

        while ((size = fread (buffer, 1, sizeof buffer / 1, file)) > 0)
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

  output = g_variant_markup_print (value, NULL, TRUE, 0, 2);
  g_variant_unref (value);

  g_print ("%s", output->str);
  g_string_free (output, TRUE);

  return 0;
}
