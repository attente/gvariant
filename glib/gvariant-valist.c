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

      if (!strchr ("bynqiuxtdsog", *(*format_string)++))         /* key */
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
        const GVariantType *type;

        type = (const GVariantType *) *format_string;
        if (!g_variant_type_string_scan (format_string, NULL))
          return FALSE;

        if (!g_variant_type_is_fixed_size (type))
          return FALSE;
      }

    case 'b': case 'y': case 'n': case 'q': case 'i': case 'u':
    case 'x': case 't': case 'd': case 's': case 'o': case 'g':
    case 'v': case '*':
      return TRUE;

    default:
      return FALSE;
  }
}

static gboolean
g_variant_format_string_is_valid (const gchar *format_string)
{
  return g_variant_format_string_scan (&format_string) &&
         *format_string == '\0';
}

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
      return g_variant_new_string (va_arg (*app, const gchar *));

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
      {
        GVariantBuilder *builder;
        const GVariantType *type;
        GVariant *value;

        type = (const GVariantType *) *format_string;
        if G_UNLIKELY (!g_variant_type_string_scan (format_string, NULL))
          g_error ("array type in format string is not valid");

        builder = va_arg (*app, GVariantBuilder *);
        value = g_variant_builder_end (builder, type);

        return value;
      }

    case 'm':
      {
        GVariantBuilder *builder;
        const gchar *string;
        GVariantType *type;
        GVariant *value;

        type = g_variant_format_string_get_type (format_string);
        builder = g_variant_builder_new (G_VARIANT_TYPE_CLASS_MAYBE, type);
        g_variant_type_free (type);

        switch (*((*format_string) + 1))
        {
          case 's':
            if ((string = va_arg (*app, const gchar *)))
              g_variant_builder_add_value (builder,
                                           g_variant_new_string (string));
            *format_string += 2;
            break;

          case 'o':
            if ((string = va_arg (*app, const gchar *)))
              g_variant_builder_add_value (builder,
                                           g_variant_new_object_path (string));
            *format_string += 2;
            break;

          case 'g':
            if ((string = va_arg (*app, const gchar *)))
              g_variant_builder_add_value (builder,
                                           g_variant_new_signature (string));
            *format_string += 2;
            break;

          case '*':
          case '@':
            if ((value = va_arg (*app, GVariant *)))
              g_variant_builder_add_value (builder, value);
            break;

          case 'v':
            if ((value = va_arg (*app, GVariant *)))
              g_variant_builder_add_value (builder, g_variant_new_variant (value));
            break;
            
          default:
            {
              gboolean just;

              just = va_arg (*app, gboolean);

              if (just)
                {
                  value = g_variant_valist_new (format_string, app);
                  g_variant_builder_add_value (builder, value);
                }
            }
        }

        return g_variant_builder_end (builder, NULL);
      }

    case '(':
      {
      }
  }
  g_error ("wtf");
}

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
  g_variant_flatten (value);

  return value;
}

void
g_variant_get_va (GVariant     *value,
                  const gchar **format_string,
                  va_list      *app)
{
  GVariantType *type;
  const gchar *fmt;

  fmt = *format_string;
  type = g_variant_format_string_get_type (format_string);
  g_assert (g_variant_matches (value, type));
}

static void
g_variant_valist_get (GVariant     *value,
                      const gchar **format_string,
                      va_list      *app)
{
  switch (g_variant_type_get_natural_class (type))
  {
    case G_VARIANT_TYPE_CLASS_BOOLEAN:
      {
        gboolean *ptr;

        ptr = va_arg (*app, gboolean *);

        if (ptr != NULL)
          {
            guint8 byte;

            g_variant_get_small (value, &byte, 1);
            *ptr = byte;
          }

        break;
      }

    case G_VARIANT_TYPE_CLASS_BYTE:
      g_variant_get_small (value, va_arg (*app, guint8 *), 1);
      break;

    case G_VARIANT_TYPE_CLASS_INT16:
    case G_VARIANT_TYPE_CLASS_UINT16:
      g_variant_get_small (value, va_arg (*app, guint16 *), 2);
      break;

    case G_VARIANT_TYPE_CLASS_INT32:
    case G_VARIANT_TYPE_CLASS_UINT32:
      g_variant_get_small (value, va_arg (*app, guint32 *), 4);
      break;

    case G_VARIANT_TYPE_CLASS_INT64:
    case G_VARIANT_TYPE_CLASS_UINT64:
    case G_VARIANT_TYPE_CLASS_DOUBLE:
      g_variant_get_small (value, va_arg (*app, guint32 *), 8);
      break;

    case G_VARIANT_TYPE_CLASS_STRING:
    case G_VARIANT_TYPE_CLASS_OBJECT_PATH:
    case G_VARIANT_TYPE_CLASS_SIGNATURE:
      {
        char **ptr;

        ptr = va_arg (*app, char **);

        if (ptr != NULL)
          *ptr = g_memdup (g_variant_get_data (value),
                           g_variant_get_size (value));

        break;
      }

    case G_VARIANT_TYPE_CLASS_ALL:
      {
        GVariant **ptr;

        ptr = va_arg (*app, GVariant **);

        if (ptr != NULL)
          *ptr = g_variant_ref (value);

        break;
      }

    case G_VARIANT_TYPE_CLASS_VARIANT:
      {
        GVariant **ptr;

        ptr = va_arg (*app, GVariant **);

        if (ptr != NULL)
          *ptr = g_variant_get_child (value, 0);

        break;
      }
    case G_VARIANT_TYPE_CLASS_ARRAY:
      {
        GVariantIter *iter;

        iter = va_arg (*app, GVariantIter *);

        if (iter)
          g_variant_iter_init (iter, value);

        break;
      }

    case G_VARIANT_TYPE_CLASS_STRUCT:
      {
        GSignature itemsig;
        int i;

        i = 0;
        itemsig = g_signature_first (signature);

        while (itemsig != NULL)
        {
          GVariant *item;

          item = g_variant_get_child (value, i++);
          g_variant_valist_get (item, free, itemsig, app);
          g_variant_unref (item);

          itemsig = g_signature_next (itemsig);
        }
        break;
      }

    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      {
        GSignature keysig, valsig;
        GVariant *key, *val;

        keysig = g_signature_key (signature);
        key = g_variant_get_child (value, 0);
        g_variant_valist_get (key, free, keysig, app);
        g_variant_unref (key);

        valsig = g_signature_value (signature);
        val = g_variant_get_child (value, 1);
        g_variant_valist_get (val, free, valsig, app);
        g_variant_unref (val);

        break;
      }

    default:
      g_assert_not_reached ();
  }
}

static GVariant *
g_variant_valist_new (GSignature  signature,
                      va_list    *app)
{
  switch (g_signature_type (signature))
  {
    case G_VARIANT_TYPE_CLASS_BOOLEAN:
      {
        guint8 byte = !!va_arg (*app, gboolean);
        return g_variant_new_small (signature, &byte, 1);
      }

    case G_VARIANT_TYPE_CLASS_BYTE:
      {
        guint8 byte = va_arg (*app, guint);
        return g_variant_new_small (signature, &byte, 1);
      }

    case G_VARIANT_TYPE_CLASS_INT16:
    case G_VARIANT_TYPE_CLASS_UINT16:
      {
        guint16 integer_16 = va_arg (*app, guint);
        return g_variant_new_small (signature, &integer_16, 2);
      }

    case G_VARIANT_TYPE_CLASS_INT32:
    case G_VARIANT_TYPE_CLASS_UINT32:
      {
        guint32 integer_32 = va_arg (*app, guint);
        return g_variant_new_small (signature, &integer_32, 4);
      }

    case G_VARIANT_TYPE_CLASS_INT64:
    case G_VARIANT_TYPE_CLASS_UINT64:
      {
        guint64 integer_64 = va_arg (*app, guint64);
        return g_variant_new_small (signature, &integer_64, 8);
      }

    case G_VARIANT_TYPE_CLASS_DOUBLE:
      {
        double floating = va_arg (*app, double);
        return g_variant_new_small (signature, &floating, 8);
      }

    case G_VARIANT_TYPE_CLASS_STRING:
      return g_variant_new_string (va_arg (*app, const char *));

    case G_VARIANT_TYPE_CLASS_OBJECT_PATH:
      return g_variant_new_object_path (va_arg (*app, const char *));

    case G_VARIANT_TYPE_CLASS_SIGNATURE:
      return g_variant_new_signature (va_arg (*app, const char *));

    case G_VARIANT_TYPE_CLASS_ANY:
      return va_arg (*app, GVariant *);

    case G_VARIANT_TYPE_CLASS_VARIANT:
      return g_variant_new_variant (va_arg (*app, GVariant *));

    case G_VARIANT_TYPE_CLASS_ARRAY:
      {
        GSVHelper *helper;
        GSignature elemsig;
        GVariant **children;
        gboolean trusted;
        int n_children;
        int i;

        n_children = va_arg (*app, int);
        g_assert_cmpint (n_children, >=, 0);

        elemsig = g_signature_element (signature);
        children = g_slice_alloc (sizeof (GVariant *) * n_children);

        trusted = TRUE;
        for (i = 0; i < n_children; i++)
          {
            children[i] = g_variant_valist_new (elemsig, app);
            trusted &= g_variant_is_normalised (children[i]);
          }

        if (g_signature_concrete (signature))
          helper = g_svhelper_get (signature);
        else
          {
            GSignature array_sig;

            if (n_children == 0)
              g_error ("g_variant_new error: non-concrete type given for "
                       "array and unable to infer type from an array "
                       "containing zero elements");

            elemsig = g_variant_get_signature (children[0]);
            for (i = 1; i < n_children; i++)
              if (!g_variant_matches (children[i], elemsig))
                g_error ("g_variant_new error: arrays must contain elements "
                         "with the same signature, but element %d has a "
                         "signature different than %s.", i,
                         i == 1 ? "element 0" : "the elements before it");

            array_sig = g_signature_arrayify (elemsig);
            helper = g_svhelper_get (array_sig);
            g_signature_free (array_sig);
          }

        return g_variant_new_tree (helper, children, n_children, trusted);
      }

    case G_VARIANT_TYPE_CLASS_STRUCT:
      {
        GSVHelper *helper;
        GSignature itemsig;
        GVariant **children;
        gboolean trusted;
        gsize n_children;
        int i;

        n_children = g_signature_items (signature);
        children = g_slice_alloc (sizeof (GVariant *) * n_children);
        itemsig = g_signature_first (signature);

        trusted = TRUE;
        for (i = 0; i < n_children; i++)
          {
            children[i] = g_variant_valist_new (itemsig, app);
            trusted &= g_variant_is_normalised (children[i]);
            itemsig = g_signature_next (itemsig);
          }

        g_assert (itemsig == NULL);

        if (g_signature_concrete (signature))
          helper = g_svhelper_get (signature);
        else
          {
            GSignature *signatures;
            GSignature struct_sig;

            signatures = g_alloca (sizeof (GSignature) * i);
            for (i = 0; i < n_children; i++)
              signatures[i] = g_variant_get_signature (children[i]);

            struct_sig = g_signature_structify (signatures,  n_children);
            helper = g_svhelper_get (struct_sig);
            g_signature_free (struct_sig);
          }

        return g_variant_new_tree (helper, children, n_children, trusted);
      }

    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      {
        GSVHelper *helper;
        GVariant **children;

        children = g_slice_alloc (sizeof (GVariant *) * 2);
        children[0] = g_variant_valist_new (g_signature_key (signature), app);
        children[1] = g_variant_valist_new (g_signature_value (signature), app);

        if (g_signature_concrete (signature))
          helper = g_svhelper_get (signature);
        else
          {
            GSignature key_sig, val_sig, entry_sig;

            key_sig = g_variant_get_signature (children[0]);
            val_sig = g_variant_get_signature (children[1]);
            entry_sig = g_signature_dictify (key_sig, val_sig);
            helper = g_svhelper_get (entry_sig);
            g_signature_free (entry_sig);
          }

        return g_variant_new_tree (helper, children, 2,
                                   g_variant_is_normalised (children[0]) &&
                                   g_variant_is_normalised (children[1]));
      }

    default:
      g_error ("valist_new unhandled: %c", g_signature_type (signature));
  }
}

#undef va_ref
#ifdef G_VA_COPY_AS_ARRAY
#define va_ref(x) ((va_list *) x)
#else
#define va_ref(x) (&(x))
#endif

void
g_variant_vvget (GVariant   *value,
                 gboolean    free,
                 GSignature  signature,
                 va_list    *app)
{
  g_assert (g_variant_matches (value, signature));

  g_variant_flatten (value);
  g_variant_valist_get (value, free, signature, app);
}

void
g_variant_vget (GVariant   *value,
                gboolean    free,
                GSignature  signature,
                va_list     ap)
{
  g_assert (g_variant_matches (value, signature));

  g_variant_flatten (value);
  g_variant_valist_get (value, free, signature, va_ref (ap));
}

void
g_variant_get_full (GVariant   *value,
                    gboolean    free,
                    GSignature  signature,
                    ...)
{
  va_list ap;

  g_assert (g_variant_matches (value, signature));

  g_variant_flatten (value);
  va_start (ap, signature);
  g_variant_valist_get (value, free, signature, &ap);
  va_end (ap);
}

/**
 * g_variant_get:
 * @value: a #GVariant
 * @signature_string: a #GSignature string
 * @...: position paramaters as per
 *   @signature_string
 *
 * Deconstructs a #GVariant.
 *
 * This function does the reverse job of
 * g_varient_new(); think of it as an analogue to
 * sscanf().
 *
 * @signature_string must be a valid #GSignature
 * string.  @value must match the string.  If you
 * are unsure if @value is a match, check first
 * with g_variant_matches ().
 *
 * The signature is scanned according to the same
 * rules as g_variant_new() except that instead of
 * expecting the paramaters to have the equivalent
 * C type of the encountered character, the
 * parameters are expected to be pointers to
 * variables of the equivalent type.  As with
 * sscanf(), the variables at the end of these
 * pointers are used to store the values resulting
 * from the deconstruction.  If a %NULL pointer is
 * given then that variable is skipped.  As with
 * g_variant_new() arrays are handled differently;
 * see below.
 *
 * The caller owns a reference to any #GVariant
 * values that are returned.  The caller is
 * responsible for calling g_free() on any string
 * values (including object paths and signatures)
 * that are returned.
 *
 * @value is not destroyed or unreffed.
 *
 * As a clatifying example:
 * <programlisting>
 * g_variant_get (value, "*", &value2);
 * </programlisting>
 *
 * is exactly equivalent to:
 * <programlisting>
 * value2 = g_variant_ref (value);
 * </programlisting>
 *
 * Another simple example is exactly equivalent to
 * g_variant_get_int32:
 * <programlisting>
 * gint32 size;
 *
 * g_variant_get (value, "i", &size);
 * </programlisting>
 *
 * The following example is more useful.  It
 * deconstructs the 'value' constructed in the
 * example under g_variant_new():
 * <programlisting>
 * int width, height;
 * GVariant *variant;
 * char *title;
 *
 * g_variant_get (value, "(s(ii)v)", &title, &width, &height, &variant);
 * ... do something useful ...
 * g_variant_unref (variant);
 * g_free (title);
 * </programlisting>
 *
 * When an 'a' is encountered in the signature,
 * the type of the array is entirely ignored
 * (except that, if given, it must match the
 * value).  A #GVariantIter is expected which is
 * initialised with the array.
 *
 * The following example deconstructs the 'value'
 * constructed in the array example for
 * g_variant_new().  It uses a %NULL pointer to
 * ignores the 'outside of the array' string:
 * <programlisting>
 * GVariantIter iter;
 * char *name;
 * gint32 number;
 *
 * g_variant_get (value, "(a*s)", iter, NULL);
 * while (g_variant_iterate (iter, "{si}", &name, &number))
 *   printf ("  '%s' -> %d\n", name, number);
 * </programlisting>
 **/
void
g_variant_get (GVariant   *value,
               const char *signature_string,
               ...)
{
  GSignature signature;
  va_list ap;

  signature = g_signature (signature_string);
  g_assert (g_variant_matches (value, signature));

  g_variant_flatten (value);
  va_start (ap, signature_string);
  g_variant_valist_get (value, FALSE, signature, &ap);
  va_end (ap);
}

GVariant *
g_variant_vvnew (GSignature  signature,
                 va_list    *app)
{
  GVariant *value;

  value = g_variant_valist_new (signature, app);
  g_variant_flatten (value);

  return g_variant_ensure_floating (value);
}

GVariant *
g_variant_vnew (GSignature signature,
                va_list    ap)
{
  GVariant *value;

  value = g_variant_valist_new (signature, &ap);
  g_variant_flatten (value);

  return g_variant_ensure_floating (value);
}

GVariant *
g_variant_new_full (GSignature signature,
                    ...)
{
  GVariant *value;
  va_list ap;

  va_start (ap, signature);
  value = g_variant_valist_new (signature, &ap);
  va_end (ap);

  g_variant_flatten (value);

  return g_variant_ensure_floating (value);
}

/**
 * g_variant_new:
 * @signature_string: a #GSignature string
 * @...: position paramaters as per
 *   @signature_string
 * @returns: a new #GVariant
 *
 * Constructs a #GVariant.
 *
 * Think of this function as an analogue of
 * g_strdup_printf().  It creates a
 * freshly-allocated #GVariant instance from the
 * pattern given by @signature_string and a
 * corresponding number of positional parameters.
 *
 * @signature_string must be a valid signature
 * string (as per #GSignature).
 *
 * Any brackets in the signature string are used
 * to represent structure and do not correspond to
 * any positional parameters.  Every other
 * character as encountered from left to right
 * expects a single positional parameter (with the
 * exception of 'a'; see below).
 *
 * The type of the expected parameter for a given
 * character is the equivalent C type of a
 * #GVariant with that character as its signature.
 * In the case of a '*' appearing in the signature
 * string, a pointer to a #GVariant is expected,
 * which is used directly as the value for that
 * position.  Any #GVariant that is passed in this
 * way (and also those expected by 'v' characters)
 * has the caller's reference count assumed by
 * g_variant_new().
 *
 * As a clarifying example, the following call
 * does absolutely nothing (assuming 'value' is a
 * valid #GVariant instance):
 * <programlisting>
 * value = g_variant_new ("*", value);
 * </programlisting>
 *
 * Another simple example is equivlanet to
 * g_variant_new_int32:
 * <programlisting>
 * value = g_variant_new ("i", 800);
 * </programlisting>
 *
 * The following is slightly more interesting:
 * <programlisting>
 * value = g_variant_new ("(s(ii)v)", "Hello World", 800, 600
 *                        g_variant_new_boolean (TRUE));
 * </programlisting>
 *
 * Arrays are handled differently.  In the case
 * that an 'a' character is encountered then an
 * integer is first read, followed by that many
 * instances of the element type for the array.
 *
 * For example:
 * <programlisting>
 * value = g_variant_new ("(a{si}s)",
 *                        3,
 *                          "zero", 0,
 *                          "one",  1,
 *                          "two",  2,
 *                        "outside of the array");
 * </programlisting>
 *
 * If you would like to construct an array less
 * directly then use the '*' character to capture
 * a #GVariant parameter.  The following example
 * produces exactly the same value as the one
 * before:
 *
 * <programlisting>
 * array = g_variant_new ("a{si}", 3, "zero", 0, "one", 1, "two",  2);
 * value = g_variant_new ("(*s)", array, "outside of the array");
 * </programlisting>
 *
 * (note that the caller's reference to 'array' is
 * consumed by the second call)
 **/
GVariant *
g_variant_new (const char *signature_string,
               ...)
{
  GSignature signature;
  GVariant *value;
  va_list ap;

  signature = g_signature (signature_string);

  va_start (ap, signature_string);
  value = g_variant_valist_new (signature, &ap);
  va_end (ap);

  g_variant_flatten (value);

  return g_variant_ensure_floating (value);
}

/**
 * g_variant_iterate:
 * @iter: a #GVariantIter
 * @signature_string: a #GSignature string
 * @returns: %TRUE if a child was fetched or
 *   %FALSE if no children remain.
 *
 * Retreives the next child value from @iter and
 * deconstructs it according to @signature_string.
 *
 * This call is essentially equivalent to
 * g_variant_iter_next() and g_variant_get().  On
 * all but the first access to the iterator the
 * given pointers are freed, if appropriate.
 *
 * It might be used as follows:
 *
 * <programlisting>
 * {
 *   GVariantIter iter;
 *   char *key, *value;
 *   ...
 *
 *   while (g_variant_iterate (iter, "{ss}", &key, &value))
 *     printf ("dict{%s} = %s\n", key, value);
 * }
 * </programlisting>
 *
 * Note that on each time through the loop 'key'
 * and 'value' are newly allocated strings.  They
 * do not need to be freed, however, since the
 * next call to g_variant_iterate will free them.
 *
 * Before the first iteration of the loop, the
 * pointers are not expected to be initialised to
 * any particular value.  After the last iteration
 * the pointers will be set to %NULL.
 *
 * If you wish to 'steal' the newly allocated
 * strings for yourself then you must set the
 * pointers to %NULL before the next call to
 * g_variant_iterate() to prevent them from being
 * freed.
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
  g_variant_vget (value, format_string, ap);
  va_end (ap);

  g_variant_unref (value);

  return TRUE;
}


void
g_variant_builder_add (GVariantBuilder *builder,
                       const gchar     *format_string,
                       ...)
{
  GVariant *variant;
  va_list ap;

  va_start (ap, format_string);
  variant = g_variant_vnew (format_string, ap);
  va_end (ap);

  g_variant_builder_add_value (builder, variant);
}


