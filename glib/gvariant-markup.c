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
  const GVariantType *type;

  type = g_variant_get_type (value);

  if (G_UNLIKELY (string == NULL))
    string = g_string_new (NULL);

  indentation += tabstop;
  g_variant_markup_indent (string, indentation);

  switch (g_variant_get_type_class (value))
  {
    case G_VARIANT_TYPE_CLASS_VARIANT:
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

    case G_VARIANT_TYPE_CLASS_MAYBE:
      {
        if (g_variant_n_children (value))
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
        else
          g_string_append_printf (string, "<nothing type='%s'/>",
                                  g_variant_get_type_string (value));

        break;
      }

    case G_VARIANT_TYPE_CLASS_ARRAY:
      {
        GVariantIter iter;

        if (g_variant_iter_init (&iter, value))
          {
            GVariant *element;

            g_string_append (string, "<array>");
            g_variant_markup_newline (string, newlines);

            while ((element = g_variant_iter_next (&iter)))
              g_variant_markup_print (element, string,
                                      newlines, indentation, tabstop);

            g_variant_markup_indent (string, indentation);
            g_string_append (string, "</array>");
          }
        else
          g_string_append_printf (string, "<array type='%s'/>",
                                  g_variant_get_type_string (value));

        break;
      }

    case G_VARIANT_TYPE_CLASS_STRUCT:
      {
        GVariantIter iter;

        if (g_variant_iter_init (&iter, value))
          {
            GVariant *element;

            g_string_append (string, "<struct>");
            g_variant_markup_newline (string, newlines);

            while ((element = g_variant_iter_next (&iter)))
              g_variant_markup_print (element, string,
                                      newlines, indentation, tabstop);

            g_variant_markup_indent (string, indentation);
            g_string_append (string, "</struct>");
          }
        else
          g_string_append (string, "<triv/>");

        break;
      }

    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      {
        GVariantIter iter;
        GVariant *element;

        g_string_append (string, "<dictionary-entry>");
        g_variant_markup_newline (string, newlines);

        g_variant_iter_init (&iter, value);
        while ((element = g_variant_iter_next (&iter)))
          g_variant_markup_print (element, string,
                                  newlines, indentation, tabstop);

        g_variant_markup_indent (string, indentation);
        g_string_append (string, "</dictionary-entry>");

        break;
      }

    case G_VARIANT_TYPE_CLASS_STRING:
      {
        char *escaped;

        escaped = g_markup_printf_escaped ("<string>%s</string>",
                                           g_variant_get_string (value,
                                                                 NULL));
        g_string_append (string, escaped);
        g_free (escaped);
        break;
      }

    case G_VARIANT_TYPE_CLASS_BOOLEAN:
      if (g_variant_get_boolean (value))
        g_string_append (string, "<true/>");
      else
        g_string_append (string, "<false/>");
      break;

    case G_VARIANT_TYPE_CLASS_BYTE:
      g_string_append_printf (string, "<byte>0x%02x</byte>",
                              g_variant_get_byte (value));
      break;

    case G_VARIANT_TYPE_CLASS_INT16:
      g_string_append_printf (string, "<int16>%"G_GINT16_FORMAT"</int16>",
                              g_variant_get_int16 (value));
      break;

    case G_VARIANT_TYPE_CLASS_UINT16:
      g_string_append_printf (string, "<uint16>%"G_GUINT16_FORMAT"</uint16>",
                              g_variant_get_uint16 (value));
      break;

    case G_VARIANT_TYPE_CLASS_INT32:
      g_string_append_printf (string, "<int32>%"G_GINT32_FORMAT"</int32>",
                              g_variant_get_int32 (value));
      break;

    case G_VARIANT_TYPE_CLASS_UINT32:
      g_string_append_printf (string, "<uint32>%"G_GUINT32_FORMAT"</uint32>",
                              g_variant_get_uint32 (value));
      break;

    case G_VARIANT_TYPE_CLASS_INT64:
      g_string_append_printf (string, "<int64>%"G_GINT64_FORMAT"</int64>",
                              g_variant_get_int64 (value));
      break;

    case G_VARIANT_TYPE_CLASS_UINT64:
      g_string_append_printf (string, "<uint64>%"G_GUINT64_FORMAT"</uint64>",
                              g_variant_get_uint64 (value));
      break;

    case G_VARIANT_TYPE_CLASS_DOUBLE:
      g_string_append_printf (string, "<double>%f</double>",
                              g_variant_get_double (value));
      break;

    default:
      g_error ("sorry... not handled yet: %s",
               g_variant_get_type_string (value));
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

static GVariantParseData *
g_variant_parse_data_new (const GVariantType *type)
{
  GVariantParseData *data;

  data = g_slice_new (GVariantParseData);
  data->builder = g_variant_builder_new (G_VARIANT_TYPE_CLASS_VARIANT, type);
  data->terminal_value = FALSE;
  data->string = NULL;

  return data;
}

static void
g_variant_parse_data_free (gpointer _data)
{
  GVariantParseData *data = _data;

  if (data->builder)
    g_variant_builder_cancel (data->builder);

  if (data->string)
    g_string_free (data->string, TRUE);

  g_slice_free (GVariantParseData, data);
}

static GVariant *
g_variant_parse_data_end (GVariantParseData  *data,
                          gboolean            and_free,
                          GError            **error)
{
  GVariant *variant, *value;

  g_assert (data != NULL);
  g_assert (data->builder != NULL);
  g_assert (data->string == NULL);

  /* we're sure that the tag stack is balanced since
   * GMarkup wouldn't let us end otherwise...
   */
  if (!g_variant_builder_check_end (data->builder, error))
    return NULL;

  variant = g_variant_builder_end (data->builder);
  data->builder = NULL;

  value = g_variant_get_child (variant, 0);
  g_variant_unref (variant);

  if (and_free)
    g_variant_parse_data_free (data);

  return value;
}

static GVariantTypeClass
type_class_from_keyword (const char *keyword)
{
  struct keyword_mapping
  {
    GVariantTypeClass class;
    const char *keyword;
  } list[] = {
    { G_VARIANT_TYPE_CLASS_BOOLEAN,          "boolean" },
    { G_VARIANT_TYPE_CLASS_BYTE,             "byte" },
    { G_VARIANT_TYPE_CLASS_INT16,            "int16" },
    { G_VARIANT_TYPE_CLASS_UINT16,           "uint16" },
    { G_VARIANT_TYPE_CLASS_INT32,            "int32" },
    { G_VARIANT_TYPE_CLASS_UINT32,           "uint32" },
    { G_VARIANT_TYPE_CLASS_INT64,            "int64" },
    { G_VARIANT_TYPE_CLASS_UINT64,           "uint64" },
    { G_VARIANT_TYPE_CLASS_DOUBLE,           "double" },
    { G_VARIANT_TYPE_CLASS_STRING,           "string" },
    { G_VARIANT_TYPE_CLASS_OBJECT_PATH,      "object-path" },
    { G_VARIANT_TYPE_CLASS_SIGNATURE,        "signature" },
    { G_VARIANT_TYPE_CLASS_VARIANT,          "variant" },
    { G_VARIANT_TYPE_CLASS_MAYBE,            "maybe" },
    { G_VARIANT_TYPE_CLASS_MAYBE,            "nothing" },
    { G_VARIANT_TYPE_CLASS_ARRAY,            "array" },
    { G_VARIANT_TYPE_CLASS_STRUCT,           "struct" },
    { G_VARIANT_TYPE_CLASS_DICT_ENTRY,       "dictionary-entry" }
  };
  gint i;

  for (i = 0; i < G_N_ELEMENTS (list); i++)
    if (!strcmp (keyword, list[i].keyword))
      return list[i].class;

  return G_VARIANT_TYPE_CLASS_INVALID;
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
  const gchar *type_string;
  const GVariantType *type;
  GVariantTypeClass class;
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
                                        g_variant_get_type_class (value),
                                        g_variant_get_type (value), error))
        return;

      g_variant_builder_add_value (data->builder, value);
      data->terminal_value = TRUE;

      return;
    }

  class = type_class_from_keyword (element_name);

  if (class == G_VARIANT_TYPE_CLASS_INVALID)
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
                                    "type", &type_string,
                                    G_MARKUP_COLLECT_INVALID))
    return;

  if (type_string && !g_variant_type_string_is_valid (type_string))
    {
      g_set_error (error, 0, 0, /* XXX FIXME */
                   "'%s' is not a valid type string", type_string);
      return;
    }

  type = type_string ? G_VARIANT_TYPE (type_string) : NULL;

  if (!g_variant_builder_check_add (data->builder, class, type, error))
    return;

  if (g_variant_type_class_is_basic (class))
    data->string = g_string_new (NULL);
  else
    data->builder = g_variant_builder_open (data->builder, class, type);

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
  GVariantTypeClass class;

  if (data->terminal_value)
    {
      data->terminal_value = FALSE;
      return;
    }

  class = type_class_from_keyword (element_name);

  if (g_variant_type_class_is_basic (class))
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
          class != G_VARIANT_TYPE_CLASS_STRING)
        {
          g_set_error (error, 0, 0, /* XXX FIXME */
                       "character data expected before </%s>",
                       element_name);
          return;
        }

      string = &data->string->str[i];

      switch (class)
      {
        case G_VARIANT_TYPE_CLASS_BOOLEAN:
          value = g_variant_new_boolean (parse_bool (string, &end));
          break;

        case G_VARIANT_TYPE_CLASS_BYTE:
          value = g_variant_new_byte (g_ascii_strtoll (string, &end, 0));
          break;

        case G_VARIANT_TYPE_CLASS_INT16:
          value = g_variant_new_int16 (g_ascii_strtoll (string, &end, 0));
          break;

        case G_VARIANT_TYPE_CLASS_UINT16:
          value = g_variant_new_uint16 (g_ascii_strtoull (string, &end, 0));
          break;

        case G_VARIANT_TYPE_CLASS_INT32:
          value = g_variant_new_int32 (g_ascii_strtoll (string, &end, 0));
          break;

        case G_VARIANT_TYPE_CLASS_UINT32:
          value = g_variant_new_uint32 (g_ascii_strtoull (string, &end, 0));
          break;

        case G_VARIANT_TYPE_CLASS_INT64:
          value = g_variant_new_int64 (g_ascii_strtoll (string, &end, 0));
          break;

        case G_VARIANT_TYPE_CLASS_UINT64:
          value = g_variant_new_uint64 (g_ascii_strtoull (string, &end, 0));
          break;

        case G_VARIANT_TYPE_CLASS_DOUBLE:
          value = g_variant_new_double (g_ascii_strtod (string, &end));
          break;

        case G_VARIANT_TYPE_CLASS_STRING:
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

  g_variant_parse_data_free (data);
}

GMarkupParser g_variant_markup_parser =
{
  g_variant_markup_parser_start_element,
  g_variant_markup_parser_end_element,
  g_variant_markup_parser_text,
  NULL,
  g_variant_markup_parser_error
};


/**
 * g_variant_markup_subparser_start:
 * @context: a #GMarkupParseContext
 * @type: a #GVariantType constraining the type of the root element
 *
 * One of the three interfaces to the #GVariant markup parser.  For
 * information about the others, see g_variant_markup_parse() and
 * g_variant_markup_parse_context_new().
 *
 * You should use this interface if you are parsing an XML document
 * using #GMarkupParser and that document contains an embedded
 * #GVariant among the markup.
 *
 * You should call this function from the start_element handler of
 * your parser for the element containing the markup for the
 * #GVariant and then return immediately.  The next call to your
 * parser will either be an error condition or a call to the
 * end_element handler for the tag matching the start tag.  From here,
 * you should call g_variant_markup_parser_pop to collect the result.
 *
 * For example, if your document contained sections like this:
 *
 * <programlisting>
 *   &lt;my-value&gt;
 *     &lt;int32&gt;42&lt;/int32&gt;
 *   &lt;/my-value&gt;
 * </programlisting>
 *
 * Then your handlers might contain code like:
 *
 * <programlisting>
 * start_element()
 * {
 *   if (strcmp (element_name, "my-value") == 0)
 *     g_variant_markup_subparser_start (context, NULL);
 *   else
 *     {
 *       ...
 *     }
 *
 * }
 *
 * end_element()
 * {
 *   if (strcmp (element_name, "my-value") == 0)
 *     {
 *       GVariant *value;
 *       
 *       if (!(value = g_variant_markup_subparser_pop (context, error)))
 *         return;
 *
 *       ...
 *     }
 *   else
 *     {
 *       ...
 *     }
 * }
 * </programlisting>
 *
 * If @type is non-%NULL then it constrains the permissible types that
 * the root element may have.  It also serves to hint the parser about
 * the type of this element (and may, for example, resolve errors
 * caused by the inability to infer the type).
 *
 * This call never fails, but it is possible that the call to
 * g_variant_markup_subparser_end() will.
 **/
void
g_variant_markup_subparser_start (GMarkupParseContext *context,
                                  const GVariantType  *type)
{
  g_markup_parse_context_push (context, &g_variant_markup_parser,
                               g_variant_parse_data_new (type));
}

/**
 * g_variant_markup_subparser_end:
 * @context: a #GMarkupParseContext
 * @error: the end_element handler @error, passed through
 * @returns: a #GVariant or %NULL.
 *
 * Ends the subparser started by g_variant_markup_subparser_start()
 * and collects the results.
 *
 * You must call this function from the end_element handler invocation
 * corresponding to the start_element handler invocation from which
 * g_variant_markup_subparser_start() was called.  This will be the
 * first end_handler invocation that is received after calling
 * g_variant_markup_subparser_start().
 *
 * If an error occured while processing tags in the subparser then
 * your end_element handler will not be invoked at all and you should
 * not call this function.
 *
 * The only time this function will fail is if no value was contained
 * between the start and ending tags.
 **/
GVariant *
g_variant_markup_subparser_end (GMarkupParseContext  *context,
                                GError              **error)
{
  return g_variant_parse_data_end (g_markup_parse_context_pop (context),
                                   TRUE, error);
}

/**
 * g_variant_markup_parse:
 * @text: the self-contained document to parse
 * @text_len: the length of @text, or -1
 * @type: a #GVariantType constraining the type of the root element
 * @error: a #GError
 * @returns: a new #GVariant, or %NULL in case of an error
 *
 * One of the three interfaces to the #GVariant markup parser.  For
 * information about the others, see
 * g_variant_markup_subparser_start() and
 * g_variant_markup_parse_context_new().
 *
 * You should use this interface if you have an XML document
 * representing a #GVariant value entirely contained within a single
 * string.
 *
 * @text should be the full text of the document.  If @text_len is not
 * -1 then it gives the length of @text (similar to
 *  g_markup_parse_context_parse()).
 *
 * If @type is non-%NULL then it constrains the permissible types that
 * the root element may have.  It also serves to hint the parser about
 * the type of this element (and may, for example, resolve errors
 * caused by the inability to infer the type).
 *
 * In the case of an error then %NULL is returned and @error is set to
 * a description of the error condition.  This function is robust
 * against arbitrary input; all error conditions are reported via
 * @error -- your program will never abort.
 **/
GVariant *
g_variant_markup_parse (const gchar         *text,
                        gssize               text_len,
                        const GVariantType  *type,
                        GError             **error)
{
  GMarkupParseContext *context;
  GVariantParseData *data;
  GVariant *value;
  GVariant *child;

  data = g_slice_new (GVariantParseData);
  data->builder = g_variant_builder_new (G_VARIANT_TYPE_CLASS_VARIANT, type);
  data->terminal_value = FALSE;
  data->string = NULL;

  context = g_markup_parse_context_new (&g_variant_markup_parser,
                                        0, data, NULL);

  if (!g_markup_parse_context_parse (context, text, text_len, error))
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

/**
 * g_variant_markup_parse_context_new:
 * @flags: #GMarkupParseFlags
 * @returns: a new #GMarkupParseContext
 *
 * One of the three interfaces to the #GVariant markup parser.  For
 * information about the others, see g_variant_markup_parse() and
 * g_variant_markup_subparser_start().
 *
 * You should use this interface if you have an XML document that you
 * want to feed to the parser in chunks.
 *
 * This call creates a #GMarkupParseContext setup for parsing a
 * #GVariant XML document.  You feed the document to the parser one
 * chunk at a time using the normal g_markup_parse_context_parse()
 * call.  After the entire document is fed, you call
 * g_variant_markup_parse_context_end() to free the context and
 * retreive the value.
 *
 * If you want to abort parsing, you should free the context using
 * g_markup_parse_context_free().
 **/
GMarkupParseContext *
g_variant_markup_parse_context_new (GMarkupParseFlags   flags,
                                    const GVariantType *type)
{
  return g_markup_parse_context_new (&g_variant_markup_parser,
                                     flags, g_variant_parse_data_new (type),
                                     &g_variant_parse_data_free);
}

/**
 * g_variant_markup_parse_context_end:
 * @context: a #GMarkupParseContext
 * @error: a #GError
 * @returns: a #GVariant, or %NULL
 *
 * Ends the parsing started with g_variant_markup_parse_context_new().
 *
 * @context must have been the result of a previous call to
 * g_variant_markup_parse_context_new().
 *
 * This function calls g_markup_parse_context_end_parse() and
 * g_markup_parse_context_free() for you.
 *
 * If the parsing was successful, a #GVariant is returned.  Otherwise,
 * %NULL is returned and @error is set accordingly.
 **/
GVariant *
g_variant_markup_parse_context_end (GMarkupParseContext  *context,
                                    GError              **error)
{
  GVariant *value = NULL;

  if (g_markup_parse_context_end_parse (context, error))
    value = g_variant_parse_data_end (
              g_markup_parse_context_get_user_data (context), FALSE, error);

  g_markup_parse_context_free (context);

  return value;
}
