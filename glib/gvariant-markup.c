#include <glib/gvariant.h>
#include <string.h>
#include <glib.h>

#include "gvariant-loadstore.h" /* XXX cheating... */

static void
g_variant_markup_indent (GString *string,
                         gint     indentation)
{
  gint i;

  for (i = 0; i < indentation; i++)
    g_string_append_c (string, ' ');
}

static void
g_variant_markup_newline (GString  *string,
                          gboolean  newlines)
{
  if (newlines)
    g_string_append_c (string, '\n');
}

GString *
g_variant_markup_print (GVariant *value,
                        GString  *string,
                        gboolean  newlines,
                        gint      indentation,
                        gint      tabstop)
{
  GSignature signature;

  signature = g_variant_get_signature (value);

  if (G_UNLIKELY (string == NULL))
    string = g_string_new (NULL);

  indentation += tabstop;
  g_variant_markup_indent (string, indentation);

  switch (g_signature_type (signature))
  {
    case G_SIGNATURE_TYPE_VARIANT:
      {
        GVariant *child;

        g_string_append (string, "<variant>");
        g_variant_markup_newline (string, newlines);

        child = g_variant_get_variant (value);
        g_variant_markup_print (child, string,
                                newlines, indentation, tabstop);
        g_variant_unref (child);

        g_variant_markup_indent (string, indentation);
        g_string_append (string, "</variant>");

        break;
      }

    case G_SIGNATURE_TYPE_MAYBE:
      {
        if (g_variant_n_children (value) == 0)
          {
            char *signature_string;

            signature_string = g_signature_get (signature);
            g_string_append_printf (string, "<nothing signature='%s'/>",
                                    signature_string);
            g_free (signature_string);
          }
        else
          {
            GVariant *element;

            g_string_append (string, "<maybe>");
            g_variant_markup_newline (string, newlines);

            element = g_variant_get_child (value, 0);
            g_variant_markup_print (element, string,
                                    newlines, indentation, tabstop);
            g_variant_unref (element);

            g_variant_markup_indent (string, indentation);
            g_string_append (string, "</maybe>");
          }

        break;
      }

    case G_SIGNATURE_TYPE_ARRAY:
      {
        GVariantIter iter;

        if (g_variant_iter_init (&iter, value))
          {
            GVariant *element;

            g_string_append (string, "<array>");
            g_variant_markup_newline (string, newlines);

            while ((element = g_variant_iter_next (&iter)))
              {
                g_variant_markup_print (element, string,
                                        newlines, indentation, tabstop);
                g_variant_unref (element);
              }

            g_variant_markup_indent (string, indentation);
            g_string_append (string, "</array>");
          }
        else
          {
            char *signature_string;

            signature_string = g_signature_get (signature);
            g_string_append_printf (string,
                                    "<array signature='%s'/>",
                                    signature_string);
            g_free (signature_string);
          }

        break;
      }

    case G_SIGNATURE_TYPE_STRUCT:
      {
        GVariantIter iter;

        if (g_variant_iter_init (&iter, value))
          {
            GVariant *element;

            g_string_append (string, "<struct>");
            g_variant_markup_newline (string, newlines);

            while ((element = g_variant_iter_next (&iter)))
              {
                g_variant_markup_print (element, string,
                                        newlines, indentation, tabstop);
                g_variant_unref (element);
              }

            g_variant_markup_indent (string, indentation);
            g_string_append (string, "</struct>");
          }
        else
          g_string_append (string, "<triv/>");

        break;
      }

    case G_SIGNATURE_TYPE_DICT_ENTRY:
      {
        GVariantIter iter;
        GVariant *element;

        g_string_append (string, "<dictionary-entry>");
        g_variant_markup_newline (string, newlines);

        g_variant_iter_init (&iter, value);
        while ((element = g_variant_iter_next (&iter)))
          {
            g_variant_markup_print (element, string,
                                    newlines, indentation, tabstop);
            g_variant_unref (element);
          }

        g_variant_markup_indent (string, indentation);
        g_string_append (string, "</dictionary-entry>");

        break;
      }

    case G_SIGNATURE_TYPE_STRING:
      {
        char *escaped;

        escaped = g_markup_printf_escaped ("<string>%s</string>",
                                           g_variant_get_string (value,
                                                                 NULL));
        g_string_append (string, escaped);
        g_free (escaped);
        break;
      }

    case G_SIGNATURE_TYPE_BOOLEAN:
      if (g_variant_get_boolean (value))
        g_string_append (string, "<true/>");
      else
        g_string_append (string, "<false/>");
      break;

    case G_SIGNATURE_TYPE_BYTE:
      g_string_append_printf (string, "<byte>0x%02x</byte>",
                              g_variant_get_byte (value));
      break;

    case G_SIGNATURE_TYPE_INT16:
      g_string_append_printf (string, "<int16>%"G_GINT16_FORMAT"</int16>",
                              g_variant_get_int16 (value));
      break;

    case G_SIGNATURE_TYPE_UINT16:
      g_string_append_printf (string, "<uint16>%"G_GUINT16_FORMAT"</uint16>",
                              g_variant_get_uint16 (value));
      break;

    case G_SIGNATURE_TYPE_INT32:
      g_string_append_printf (string, "<int32>%"G_GINT32_FORMAT"</int32>",
                              g_variant_get_int32 (value));
      break;

    case G_SIGNATURE_TYPE_UINT32:
      g_string_append_printf (string, "<uint32>%"G_GUINT32_FORMAT"</uint32>",
                              g_variant_get_uint32 (value));
      break;

    case G_SIGNATURE_TYPE_INT64:
      g_string_append_printf (string, "<int64>%"G_GINT64_FORMAT"</int64>",
                              g_variant_get_int64 (value));
      break;

    case G_SIGNATURE_TYPE_UINT64:
      g_string_append_printf (string, "<uint64>%"G_GUINT64_FORMAT"</uint64>",
                              g_variant_get_uint64 (value));
      break;

    case G_SIGNATURE_TYPE_DOUBLE:
      g_string_append_printf (string, "<double>%f</double>",
                              g_variant_get_double (value));
      break;

    default:
      g_error ("sorry... not handled yet: %s", g_signature_get (signature));
  }

  g_variant_markup_newline (string, newlines);

  return string;
}

/* parser */
typedef struct
{
  GVariantBuilder *builder;
  gboolean terminal_value;
  GString *string;
} GVariantParseData;

static GSignatureType
type_from_keyword (const char *keyword)
{
  struct keyword_mapping
  {
    GSignatureType type;
    const char *keyword;
  } list[] = {
    { G_SIGNATURE_TYPE_BOOLEAN,          "boolean" },
    { G_SIGNATURE_TYPE_BYTE,             "byte" },
    { G_SIGNATURE_TYPE_INT16,            "int16" },
    { G_SIGNATURE_TYPE_UINT16,           "uint16" },
    { G_SIGNATURE_TYPE_INT32,            "int32" },
    { G_SIGNATURE_TYPE_UINT32,           "uint32" },
    { G_SIGNATURE_TYPE_INT64,            "int64" },
    { G_SIGNATURE_TYPE_UINT64,           "uint64" },
    { G_SIGNATURE_TYPE_DOUBLE,           "double" },
    { G_SIGNATURE_TYPE_STRING,           "string" },
    { G_SIGNATURE_TYPE_OBJECT_PATH,      "object-path" },
    { G_SIGNATURE_TYPE_SIGNATURE,        "signature" },
    { G_SIGNATURE_TYPE_VARIANT,          "variant" },
    { G_SIGNATURE_TYPE_MAYBE,            "maybe" },
    { G_SIGNATURE_TYPE_MAYBE,            "nothing" },
    { G_SIGNATURE_TYPE_ARRAY,            "array" },
    { G_SIGNATURE_TYPE_STRUCT,           "struct" },
    { G_SIGNATURE_TYPE_DICT_ENTRY,       "dictionary-entry" }
  };
  gint i;

  for (i = 0; i < G_N_ELEMENTS (list); i++)
    if (!strcmp (keyword, list[i].keyword))
      return list[i].type;

  return G_SIGNATURE_TYPE_INVALID;
}

static GVariant *
value_from_keyword (const char *keyword)
{

  if (!strcmp (keyword, "true"))
    return g_variant_new_boolean (TRUE);

  else if (!strcmp (keyword, "false"))
    return g_variant_new_boolean (FALSE);

  else if (!strcmp (keyword, "triv"))
    return g_variant_new ("()");

  return NULL;
}

static void
g_variant_markup_parser_start_element (GMarkupParseContext  *context,
                                       const char           *element_name,
                                       const char          **attribute_names,
                                       const char          **attribute_values,
                                       gpointer              user_data,
                                       GError              **error)
{
  GVariantParseData *data = user_data;
  const char *signature_string;
  GSignature signature;
  GSignatureType type;
  GVariant *value;

  if (data->string != NULL)
    {
      g_set_error (error, 0, 0, /* XXX FIXME */
                   "only character data may appear here (not <%s>)",
                   element_name);
      return;
    }

  if (data->terminal_value)
    {
      g_set_error (error, 0, 0, /* XXX FIXME */
                   "nothing may appear here except </%s>",
                   (const gchar *)
                   g_markup_parse_context_get_element_stack (context)
                     ->next->data);
      return;
    }

  if ((value = value_from_keyword (element_name)))
    {
      if (!g_variant_builder_check_add (data->builder,
                                        g_signature_type (g_variant_get_signature (value)),
                                        g_variant_get_signature (value),
                                        error))
        return;

      g_variant_builder_add_value (data->builder, value);
      data->terminal_value = TRUE;

      return;
    }

  type = type_from_keyword (element_name);

  if (type == G_SIGNATURE_TYPE_INVALID)
    {
      g_set_error (error, 0, 0, /* XXX FIXME */
                   "the <%s> tag is unrecognised", element_name);
      return;
    }

  if (!g_markup_collect_attributes (element_name,
                                    attribute_names, attribute_values,
                                    error,
                                    G_MARKUP_COLLECT_OPTIONAL |
                                      G_MARKUP_COLLECT_STRING,
                                    "signature", &signature_string,
                                    G_MARKUP_COLLECT_INVALID))
    return;

  if (signature_string && !g_signature_is_valid (signature_string))
    {
      g_set_error (error, 0, 0, /* XXX FIXME */
                   "'%s' is not a valid signature string", signature_string);
      return;
    }

  signature = signature_string ? g_signature (signature_string) : NULL;

  if (!g_variant_builder_check_add (data->builder, type, signature, error))
    return;

  if (g_signature_type_is_basic (type))
    data->string = g_string_new (NULL);
  else
    data->builder = g_variant_builder_open (data->builder, type, signature);

  /* special case: <nothing/> may contain no children */
  if (strcmp (element_name, "nothing") == 0)
    {
      data->builder = g_variant_builder_close (data->builder);
      data->terminal_value = TRUE;
    }
}

static gboolean
parse_bool (char *str, char **end)
{
  if (!strncmp (str, "true", 4))
    {
      *end = str + 4;
      return TRUE;
    }
  else if (!strncmp (str, "false", 4))
    {
      *end = str + 5;
      return FALSE;
    }
  else
    {
      *end = str;
      return FALSE;
    }
}

static void
g_variant_markup_parser_end_element (GMarkupParseContext  *context,
                                     const char           *element_name,
                                     gpointer              user_data,
                                     GError              **error)
{
  GVariantParseData *data = user_data;
  GSignatureType type;

  if (data->terminal_value)
    {
      data->terminal_value = FALSE;
      return;
    }

  type = type_from_keyword (element_name);

  if (g_signature_type_is_basic (type))
    {
      GVariant *value;
      char *string;
      char *end;
      int i;

      g_assert (data->string);

      /* ensure at least one non-whitespace character */
      for (i = 0; i < data->string->len; i++)
        if (!g_ascii_isspace (data->string->str[i]))
          break;

      if (G_UNLIKELY (i == data->string->len) &&
          type != G_SIGNATURE_TYPE_STRING)
        {
          g_set_error (error, 0, 0, /* XXX FIXME */
                       "character data expected before </%s>",
                       element_name);
          return;
        }

      string = &data->string->str[i];

      switch (type)
      {
        case G_SIGNATURE_TYPE_BOOLEAN:
          value = g_variant_new_boolean (parse_bool (string, &end));
          break;

        case G_SIGNATURE_TYPE_BYTE:
          value = g_variant_new_byte (g_ascii_strtoll (string, &end, 0));
          break;

        case G_SIGNATURE_TYPE_INT16:
          value = g_variant_new_int16 (g_ascii_strtoll (string, &end, 0));
          break;

        case G_SIGNATURE_TYPE_UINT16:
          value = g_variant_new_uint16 (g_ascii_strtoull (string, &end, 0));
          break;

        case G_SIGNATURE_TYPE_INT32:
          value = g_variant_new_int32 (g_ascii_strtoll (string, &end, 0));
          break;

        case G_SIGNATURE_TYPE_UINT32:
          value = g_variant_new_uint32 (g_ascii_strtoull (string, &end, 0));
          break;

        case G_SIGNATURE_TYPE_INT64:
          value = g_variant_new_int64 (g_ascii_strtoll (string, &end, 0));
          break;

        case G_SIGNATURE_TYPE_UINT64:
          value = g_variant_new_uint64 (g_ascii_strtoull (string, &end, 0));
          break;

        case G_SIGNATURE_TYPE_DOUBLE:
          value = g_variant_new_double (g_ascii_strtod (string, &end));
          break;

        case G_SIGNATURE_TYPE_STRING:
          value = g_variant_new_string (data->string->str);
          end = (char *) "";
          break;

        default:
          g_assert_not_reached ();
      }

      /* ensure only trailing whitespace */
      for (i = 0; end[i]; i++)
        if (G_UNLIKELY (!g_ascii_isspace (end[i])))
          {
            g_set_error (error, 0, 0, /* XXX FIXME */
                         "cannot interpret character data");
            g_variant_unref (value);
            return;
          }

      g_variant_builder_add_value (data->builder, value);
      g_string_free (data->string, TRUE);
      data->string = NULL;
    }
  else
    {
      if (!g_variant_builder_check_end (data->builder, error))
        return;

      data->builder = g_variant_builder_close (data->builder);
    }
}

static void
g_variant_markup_parser_text (GMarkupParseContext  *context,
                              const char           *text,
                              gsize                 text_len,
                              gpointer              user_data,
                              GError              **error)
{
  GVariantParseData *data = user_data;

  if (data->string == NULL)
    {
      int i;

      for (i = 0; i < text_len; i++)
        if (G_UNLIKELY (!g_ascii_isspace (text[i])))
          {
            g_set_error (error, 0, 0, /* XXX FIXME */
                         "character data ('%c') is invalid here", text[i]);
            break;
          }

      return;
    }
  else
    g_string_append_len (data->string, text, text_len);
}

static void
g_variant_markup_parser_error (GMarkupParseContext *context,
                               GError              *error,
                               gpointer             user_data)
{
  GVariantParseData *data = user_data;

  if (data->string)
    g_string_free (data->string, TRUE);
  data->string = NULL;

  if (data->builder)
    g_variant_builder_abort (data->builder);
  data->builder = NULL;

  g_slice_free (GVariantParseData, data);
}

GMarkupParser g_variant_markup_parser =
{
  g_variant_markup_parser_start_element,
  g_variant_markup_parser_end_element,
  g_variant_markup_parser_text,
  NULL,
  g_variant_markup_parser_error
};

void
g_variant_markup_parser_start_parse (GMarkupParseContext *context,
                                     GSignature           signature)
{
  GVariantParseData *data;

  data = g_slice_new (GVariantParseData);
  data->builder = g_variant_builder_new (G_SIGNATURE_TYPE_VARIANT,
                                         signature);
  data->terminal_value = FALSE;
  data->string = NULL;

  g_markup_parse_context_push (context, &g_variant_markup_parser, data);
}

GVariant *
g_variant_markup_parser_end_parse (GMarkupParseContext  *context,
                                   GError              **error)
{
  GVariantParseData *data;
  GVariant *value, *child;

  data = g_markup_parse_context_pop (context);

  g_assert (data != NULL);
  g_assert (data->builder != NULL);

  if (!g_variant_builder_check_end (data->builder, error))
    return NULL;

  g_assert (data->string == NULL);
  value = g_variant_builder_end (data->builder);
  g_slice_free (GVariantParseData, data);

  child = g_variant_get_child (value, 0);
  g_variant_unref (value);

  return child;
}

GVariant *
g_variant_markup_parse (const char  *string,
                        GSignature   signature,
                        GError     **error)
{
  GMarkupParseContext *context;
  GVariantParseData *data;
  GVariant *value;
  GVariant *child;

  data = g_slice_new (GVariantParseData);
  data->builder = g_variant_builder_new (G_SIGNATURE_TYPE_VARIANT,
                                         signature);
  data->terminal_value = FALSE;
  data->string = NULL;

  context = g_markup_parse_context_new (&g_variant_markup_parser,
                                        0, data, NULL);

  if (!g_markup_parse_context_parse (context, string, -1, error))
    return FALSE;

  if (!g_markup_parse_context_end_parse (context, error))
    return FALSE;

  g_markup_parse_context_free (context);

  g_assert (data != NULL);
  g_assert (data->builder != NULL);

  if (!g_variant_builder_check_end (data->builder, error))
    return NULL;

  g_assert (data->string == NULL);
  value = g_variant_builder_end (data->builder);
  g_slice_free (GVariantParseData, data);

  child = g_variant_get_child (value, 0);
  g_variant_unref (value);

  return child;
}
