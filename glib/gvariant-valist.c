/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#include "gvariant-private.h"

#include <glib/gtestutils.h>
#include <glib/gmessages.h>
#include <string.h>

/**
 * g_variant_format_string_scan:
 * @format_string: a pass-by-reference pointer to the start of a
 *                 possible format string
 * @returns: %TRUE if a format string was scanned
 *
 * Checks the string pointed to by @format_string for starting with a
 * properly formed #GVariant varargs format string.  If a format
 * string is fount, @format_string is updated to point to the first
 * character following the format string and %TRUE is returned.
 *
 * If no valid format string is found, %FALSE is returned and the
 * state of the @format_string pointer is undefined.
 *
 * All valid #GVariantType strings are also valid format strings.  See
 * g_variant_type_string_is_valid().
 *
 * Additionally, any type string contained in the format string may be
 * prefixed with a '@' character.  Nested '@' characters may not
 * appear.
 *
 * Additionally, any fixed-width type may be prefixed with a '&'
 * character.  No wildcard type is a fixed-width type.  Like '@', '&'
 * characters may not be nested.
 *
 * No '@' or '&' character, however, may appear as part of an array
 * type.
 *
 * Currently, there are no other permissible format strings.  Others
 * may be added in the future.
 *
 * For an explanation of what these strings mean, see g_variant_new()
 * and g_variant_get().
 **/
gboolean
g_variant_format_string_scan (const gchar **format_string)
{
  switch (*(*format_string)++)
  {
    case '\0':
      return FALSE;

    case '(':
      while (**format_string != ')')
        if (!g_variant_format_string_scan (format_string))
          return FALSE;

      (*format_string)++; /* ')' */

      return TRUE;

    case '{':
      if (**format_string == '\0')
        return FALSE;

      /* key may only be a basic type.  three posibilities for that:
       */
      if (strchr ("bynqiuxtdsog?", **format_string))
        *format_string += 1;

      else if ((*format_string)[0] == '@' &&
               strchr ("bynqiuxtdsog?", (*format_string)[1]))
        *format_string += 2;

      else if ((*format_string)[0] == '&' &&
               strchr ("bynqiuxtdsog", (*format_string)[1]))
        *format_string += 2;

      else
        return FALSE;

      if (!g_variant_format_string_scan (format_string))    /* value */
        return FALSE;

      if (*(*format_string)++ != '}')
        return FALSE;

      return TRUE;

    case 'm':
      return g_variant_format_string_scan (format_string);

    case 'a':
    case '@':
      return g_variant_type_string_scan (format_string, NULL);

    case '&':
      {
        const gchar *start;

        start = *format_string;
        if (!g_variant_type_string_scan (format_string, NULL))
          return FALSE;

        if (start + strspn (start, "bynqiuxtd(){}") != *format_string)
          return FALSE;
      }

    case 'b': case 'y': case 'n': case 'q': case 'i': case 'u':
    case 'x': case 't': case 'd': case 's': case 'o': case 'g':
    case 'v': case '*': case '?':
      return TRUE;

    default:
      return FALSE;
  }
}

#if 0
static gboolean
g_variant_format_string_is_valid (const gchar *format_string)
{
  return g_variant_format_string_scan (&format_string) &&
         *format_string == '\0';
}
#endif

static GVariantType *
g_variant_format_string_get_type (const gchar **format_string)
{
  const gchar *src;
  gchar *dest;
  gchar *new;

  src = *format_string;
  if G_UNLIKELY (!g_variant_format_string_scan (format_string))
    g_error ("format string is invalid");

  dest = new = g_malloc (*format_string - src + 1);
  while (src != *format_string)
    {
      if (*src != '@' && *src != '&')
        *dest++ = *src;
      src++;
    }
  *dest = '\0';

  return (GVariantType *) G_VARIANT_TYPE (new);
}

static GVariant *
g_variant_valist_new (const gchar **format_string,
                      va_list      *app)
{
  switch (**format_string)
  {
    case 'b':
      (*format_string)++;
      return g_variant_new_boolean (va_arg (*app, gboolean));

    case 'y':
      (*format_string)++;
      return g_variant_new_byte (va_arg (*app, guint));

    case 'n':
      (*format_string)++;
      return g_variant_new_int16 (va_arg (*app, gint));

    case 'q':
      (*format_string)++;
      return g_variant_new_uint16 (va_arg (*app, guint));

    case 'i':
      (*format_string)++;
      return g_variant_new_int32 (va_arg (*app, gint));

    case 'u':
      (*format_string)++;
      return g_variant_new_uint32 (va_arg (*app, guint));

    case 'x':
      (*format_string)++;
      return g_variant_new_int64 (va_arg (*app, gint64));

    case 't':
      (*format_string)++;
      return g_variant_new_uint64 (va_arg (*app, guint64));

    case 'd':
      (*format_string)++;
      return g_variant_new_double (va_arg (*app, gdouble));

    case 's':
      (*format_string)++;
      {
        const char *karstar = va_arg (*app, const gchar *);
        g_print ("i see '%s'\n", karstar);
      return g_variant_new_string (karstar);
      }

    case 'o':
      (*format_string)++;
      return g_variant_new_object_path (va_arg (*app, const gchar *));

    case 'g':
      (*format_string)++;
      return g_variant_new_signature (va_arg (*app, const gchar *));

    case 'v':
      (*format_string)++;
      return g_variant_new_variant (va_arg (*app, GVariant *));

    case '*':
      (*format_string)++;
      return va_arg (*app, GVariant *);

    case 'a':
      return g_variant_builder_end (va_arg (*app, GVariantBuilder *));

    case 'm':
      {
        GVariantBuilder *builder;
        const gchar *string;
        GVariantType *type;
        GVariant *value;

        string = (*format_string) + 1;
        type = g_variant_format_string_get_type (format_string);
        builder = g_variant_builder_new (G_VARIANT_TYPE_CLASS_MAYBE, type);
        g_variant_type_free (type);

        switch (*((*format_string) + 1))
        {
          case 's':
            if ((string = va_arg (*app, const gchar *)))
              g_variant_builder_add_value (builder,
                                           g_variant_new_string (string));
            break;

          case 'o':
            if ((string = va_arg (*app, const gchar *)))
              g_variant_builder_add_value (builder,
                                           g_variant_new_object_path (string));
            break;

          case 'g':
            if ((string = va_arg (*app, const gchar *)))
              g_variant_builder_add_value (builder,
                                           g_variant_new_signature (string));
            break;

          case '*':
          case '@':
            if ((value = va_arg (*app, GVariant *)))
              g_variant_builder_add_value (builder, value);
            break;

          case 'v':
            if ((value = va_arg (*app, GVariant *)))
              g_variant_builder_add_value (builder,
                                           g_variant_new_variant (value));
            break;
            
          default:
            {
              gboolean just;

              just = va_arg (*app, gboolean);

              if (just)
                {
                  value = g_variant_valist_new (&string, app);
                  g_variant_builder_add_value (builder, value);
                  g_assert (string == *format_string);
                }
            }
        }

        return g_variant_builder_end (builder);
      }

    case '(':
    case '{':
      {
        GVariantBuilder *builder;

        if (**format_string == '(')
          builder = g_variant_builder_new (G_VARIANT_TYPE_CLASS_STRUCT,
                                           NULL);
        else
          builder = g_variant_builder_new (G_VARIANT_TYPE_CLASS_DICT_ENTRY,
                                           NULL);

        (*format_string)++;                                          /* '(' */
        while (**format_string != ')' && **format_string != '}')
          {
            GVariant *value;

            value = g_variant_valist_new (format_string, app);
            g_variant_builder_add_value (builder, value);
          }
        (*format_string)++;                                          /* ')' */

        return g_variant_builder_end (builder);
      }

    default:
      g_assert_not_reached ();
  }
}

static void
g_variant_valist_get (GVariant     *value,
                      gboolean      free,
                      const gchar **format_string,
                      va_list      *app)
{
#define simple_case(char, type) \
    case char:                                          \
    G_STMT_START                                        \
      {                                                 \
        g##type *ptr = va_arg (*app, g##type *);        \
        (*format_string)++;                             \
        if (ptr)                                        \
          {                                             \
            if (value)                                  \
              *ptr = g_variant_get_##type (value);      \
            else                                        \
              *ptr = 0;                                 \
          }                                             \
        return;                                         \
      }                                                 \
    G_STMT_END

  switch (**format_string)
  {
    simple_case ('b', boolean);
typedef guchar gbyte;
    simple_case ('y', byte);
    simple_case ('n', int16);
    simple_case ('q', uint16);
    simple_case ('i', int32);
    simple_case ('u', uint32);
    simple_case ('x', int64);
    simple_case ('t', uint64);
    simple_case ('d', double);

    case 's':
    case 'o':
    case 'g':
      {
        const gchar **ptr = va_arg (*app, const gchar **);
        (*format_string)++;

        if (ptr)
          *ptr = g_variant_get_string (value, NULL);
        return;
      }

    case 'v':
      {
        GVariant **ptr = va_arg (*app, GVariant **);

        if (ptr)
          {
            if (free && *ptr)
              g_variant_unref (*ptr);

            if (value)
              *ptr = g_variant_get_variant (value);
            else
              *ptr = NULL;
          }
      }

    case '*':
      {
        GVariant **ptr = va_arg (*app, GVariant **);
        (*format_string)++;

        if (ptr)
          {
            if (free && *ptr)
              g_variant_unref (*ptr);

            *ptr = g_variant_ref (value);
          }
      }

    case 'a':
      {
        GVariantIter *ptr = va_arg (*app, GVariantIter *);

        if (ptr)
          {
            if (free)
              g_variant_iter_cancel (ptr);

            g_variant_iter_init (ptr, value);
          }
      }

    case 'm':
      switch (*((*format_string) + 1))
      {
        case 's':
        case 'o':
        case 'g':
          {
            const gchar **ptr = va_arg (*app, const gchar **);

            *format_string += 2;

            if (ptr)
              {
                if (g_variant_n_children (value))
                  {
                    GVariant *child;

                    child = g_variant_get_child (value, 0);
                    *ptr = g_variant_get_string (child, NULL);
                    g_variant_unref (child);
                  }
                else
                  *ptr = NULL;
              }

            return;
          }

        case '*':
        case '@':
          {
            GVariant **ptr = va_arg (*app, GVariant **);

            g_variant_format_string_scan (format_string);

            if (ptr)
              {
                if (free && *ptr)
                  g_variant_unref (*ptr);

                if (g_variant_n_children (value))
                  *ptr = g_variant_get_child (value, 0);
                else
                  *ptr = NULL;
              }

            return;
          }

        case 'v':
          {
            GVariant **ptr = va_arg (*app, GVariant **);

            *format_string += 2;

            if (ptr)
              {
                if (free && *ptr)
                  g_variant_unref (*ptr);

                if (g_variant_n_children (value))
                  {
                    GVariant *child;

                    child = g_variant_get_child (value, 0);
                    *ptr = g_variant_get_variant (child);
                    g_variant_unref (child);
                  }
                else
                  *ptr = NULL;
              }

            return;
          }

        default:
          {
            gboolean *ptr = va_arg (*app, gboolean *);

            *format_string += 1;

            if (ptr)
              {
                if (g_variant_n_children (value))
                  {
                    GVariant *child;

                    child = g_variant_get_child (value, 0);
                    g_variant_valist_get (child, free && *ptr,
                                          format_string, app);
                    g_variant_unref (child);
                    *ptr = TRUE;
                  }
                else
                  {
                    g_variant_valist_get (NULL, free && *ptr,
                                          format_string, app);
                    *ptr = FALSE;
                  }
              }

            return;
          }
      }

    case '(':
    case '{':
      {
        GVariantIter iter;
        char end_char;

        if (**format_string == '(')
          end_char = ')';
        else
          end_char = '}';

        *format_string += 1;

        g_variant_iter_init (&iter, value);
        while ((value = g_variant_iter_next (&iter)))
          g_variant_valist_get (value, free, format_string, app);

        g_assert (**format_string == end_char);
        *format_string += 1;

        return;
      }

    default:
      g_assert_not_reached ();
  }
}

/**
 * g_variant_new:
 * @format_string: a #GVariant format string
 * @...: arguments, as per @format_string
 * @returns: a new floating #GVariant instance
 *
 * Creates a new #GVariant instance.
 *
 * Think of this function as an analogue to g_strdup_printf().
 *
 * The type of the created instance and the arguments that are
 * expected by this function are determined by @format_string.  In the
 * most simple case, @format_string is exactly equal to a concrete
 * #GVariantType type string and the result is of that type.  All
 * exceptions to this case are explicitly mentioned below.
 *
 * The arguments that this function collects are determined by
 * scanning @format_string from start to end.  Brackets do not impact
 * the collection of arguments.  Each other character that is
 * encountered will result in an argument being collected.
 *
 * Arguments for the base types are expected as follows:
 * <variablelist>
 *   <varlistentry>
 *     <term>b</term>
 *     <listitem>a #gboolean is collected</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>y</term>
 *     <listitem>a #guchar is collected</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>n</term>
 *     <listitem>a #gint16 is collected</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>q</term>
 *     <listitem>a #guint16 is collected</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>i</term>
 *     <listitem>a #gint32 is collected</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>u</term>
 *     <listitem>a #guint32 is collected</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>x</term>
 *     <listitem>a #gint64 is collected</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>t</term>
 *     <listitem>a #guint64 is collected</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>d</term>
 *     <listitem>a #gdouble is collected</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>s</term>
 *     <listitem>a non-%NULL (const #gchar *) is collected</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>o</term>
 *     <listitem>
 *       a non-%NULL (const #gchar *) is collected.  it must be a
 *       valid DBus object path.
 *     </listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>g</term>
 *     <listitem>
 *       a non-%NULL (const #gchar *) is collected.  it must be a
 *       valid DBus type signature string.
 *     </listitem>
 *   </varlistentry>
 * </variablelist>
 *
 * If a 'v' character is encountered in @format_string then a
 * (#GVariant *) is collected which must be non-%NULL and must point
 * to a valid #GVariant instance.
 *
 * If an array type is encountered in @format_string, a
 * #GVariantBuilder is collected and has g_variant_builder_end()
 * called on it.  The type of the array has no impact on argument
 * collection but is checked against the type of the array and can be
 * used to infer the type of an empty array.
 *
 * If a maybe type is encountered in @format_string, then the expected
 * arguments vary depending on the type.
 *
 * <variablelist>
 *   <varlistentry>
 *     <term>ms</term>
 *     <listitem>
 *       a possibly-%NULL (const #gchar *) is collected
 *     </listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>mo</term>
 *     <listitem>as per previous</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>mg</term>
 *     <listitem>as per previous</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>mv</term>
 *     <listitem>
 *       a possibly-%NULL (#GVariant *) is collected
 *     </listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>m*</term>
 *     <listitem>as per previous</listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>others...</term>
 *     <listitem>
 *       a #gboolean is collected.  If the collected value is %FALSE
 *       then the maybe is Nothing and the arguments corresponding to
 *       the element type of the maybe are not collected.  If %TRUE,
 *       then the arguments are collected as if there were no 'm'.
 *     </listitem>
 *   </varlistentry>
 * </variablelist>
 *
 * If a '*' character is encountered in @format_string then a
 * (#GVariant *) is collected which must be non-%NULL and must point
 * to a valid #GVariant instance.  This #GVariant is inserted directly
 * at the given position.
 *
 * Please note that the syntax of the format string is very likely to
 * be extended in the future.
 **/
GVariant *
g_variant_new (const gchar *format_string,
               ...)
{
  GVariant *value;
  va_list ap;

  va_start (ap, format_string);
  value = g_variant_new_va (&format_string, &ap);
  g_assert (*format_string == '\0');
  va_end (ap);

  return value;
}

GVariant *
g_variant_new_va (const gchar **format_string,
                  va_list      *app)
{
  GVariant *value;

  value = g_variant_valist_new (format_string, app);
//  g_variant_flatten (value);

  return value;
}

void
g_variant_get (GVariant    *value,
               const gchar *format_string,
               ...)
{
  va_list ap;

  va_start (ap, format_string);
  g_variant_get_va (value, &format_string, &ap);
  va_end (ap);
}

void
g_variant_get_va (GVariant     *value,
                  const gchar **format_string,
                  va_list      *app)
{
  GVariantType *type;
  const gchar *fmt;

  fmt = *format_string;
  type = g_variant_format_string_get_type (&fmt);
  g_assert (g_variant_matches (value, type));
  g_variant_type_free (type);

//  g_variant_flatten (value);
  g_variant_valist_get (value, FALSE, format_string, app);
}

/**
 * g_variant_iterate:
 * @iter: a #GVariantIter
 * @format_string: a format string
 * @...: arguments, as per @format_string
 * @returns: %TRUE if a child was fetched or %FALSE if not
 *
 * Retreives the next child value from @iter and deconstructs it
 * according to @format_string.  This call is sort of like calling
 * g_variant_iter_next() and g_variant_get().
 *
 * This function does something else, though: on all but the first
 * call (including on the last call, which returns %FALSE) the values
 * allocated by the previous call will be free'd.  This allows you to
 * iterate without ever freeing anything yourself.  In the case of
 * #GVariant * arguments, they are unref'd and in the case of
 * #GVariantIter arguments, they are cancelled.
 *
 * Note that strings are not free'd since (as with g_variant_get())
 * they are constant pointers to internal #GVariant data.
 *
 * This function might be used as follows:
 *
 * <programlisting>
 * {
 *   const gchar *key, *value;
 *   GVariantIter iter;
 *   ...
 *
 *   while (g_variant_iterate (iter, "{ss}", &key, &value))
 *     printf ("dict['%s'] = '%s'\n", key, value);
 * }
 * </programlisting>
 **/
gboolean
g_variant_iterate (GVariantIter *iter,
                   const gchar  *format_string,
                   ...)
{
  GVariant *value;
  va_list ap;

  value = g_variant_iter_next (iter);

  if (value == NULL)
    return FALSE;

  va_start (ap, format_string);
  g_variant_get_va (value, &format_string, &ap);
  g_assert (*format_string == '\0');
  va_end (ap);

  g_variant_unref (value);

  return TRUE;
}

/**
 * g_variant_builder_add:
 * @builder: a #GVariantBuilder
 * @format_string: a #GVariant varargs format string
 * @...: arguments, as per @format_string
 *
 * Adds to a #GVariantBuilder.
 *
 * This call is a convenience wrapper that is exactly equivalent to
 * calling g_variant_new() followed by g_variant_builder_add_value().
 * 
 * This function might be used as follows:
 *
 * <programlisting>
 * GVariant *
 * make_pointless_dictionary (void)
 * {
 *   GVariantBuilder *builder;
 *   int i;
 *
 *   builder = g_variant_builder_new (G_VARIANT_TYPE_CLASS_ARRAY, NULL);
 *   for (i = 0; i < 16; i++)
 *     {
 *       char buf[3];
 *
 *       sprintf (buf, "%d", i);
 *       g_variant_builder_add (builder, "{is}", i, buf);
 *     }
 *
 *   return g_variant_builder_end (builder);
 * }
 * </programlisting>
 **/
void
g_variant_builder_add (GVariantBuilder *builder,
                       const gchar     *format_string,
                       ...)
{
  GVariant *variant;
  va_list ap;

  va_start (ap, format_string);
  variant = g_variant_new_va (&format_string, &ap);
  g_assert (*format_string == '\0');
  va_end (ap);

  g_variant_builder_add_value (builder, variant);
}
