/* 
 * Copyright © 2007, 2008 Ryan Lortie
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 * 
 * See the included COPYING file for more information.
 */

#include <stdarg.h>
#include <glib.h>

#include "gvariant-private.h"

static void
g_variant_valist_get (GVariant   *value,
                      gboolean    free,
                      GSignature  signature,
                      va_list    *app)
{
  switch (g_signature_type (signature))
  {
    case G_SIGNATURE_TYPE_BOOLEAN:
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

    case G_SIGNATURE_TYPE_BYTE:
      g_variant_get_small (value, va_arg (*app, guint8 *), 1);
      break;

    case G_SIGNATURE_TYPE_INT16:
    case G_SIGNATURE_TYPE_UINT16:
      g_variant_get_small (value, va_arg (*app, guint16 *), 2);
      break;

    case G_SIGNATURE_TYPE_INT32:
    case G_SIGNATURE_TYPE_UINT32:
      g_variant_get_small (value, va_arg (*app, guint32 *), 4);
      break;

    case G_SIGNATURE_TYPE_INT64:
    case G_SIGNATURE_TYPE_UINT64:
    case G_SIGNATURE_TYPE_DOUBLE:
      g_variant_get_small (value, va_arg (*app, guint32 *), 8);
      break;

    case G_SIGNATURE_TYPE_STRING:
    case G_SIGNATURE_TYPE_OBJECT_PATH:
    case G_SIGNATURE_TYPE_SIGNATURE:
      {
        char **ptr;

        ptr = va_arg (*app, char **);

        if (ptr != NULL)
          *ptr = g_memdup (g_variant_get_data (value),
                           g_variant_get_size (value));

        break;
      }

    case G_SIGNATURE_TYPE_ANY:
      {
        GVariant **ptr;

        ptr = va_arg (*app, GVariant **);
        
        if (ptr != NULL)
          *ptr = g_variant_ref (value);

        break;
      }

    case G_SIGNATURE_TYPE_VARIANT:
      {
        GVariant **ptr;

        ptr = va_arg (*app, GVariant **);

        if (ptr != NULL)
          *ptr = g_variant_get_child (value, 0);

        break;
      }
    case G_SIGNATURE_TYPE_ARRAY:
      {
        GVariantIter *iter;

        iter = va_arg (*app, GVariantIter *);

        if (iter)
          g_variant_iter_init (iter, value);

        break;
      }

    case G_SIGNATURE_TYPE_STRUCT:
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

    case G_SIGNATURE_TYPE_DICT_ENTRY:
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
g_variant_valist_get_ptr (gboolean  steal,
                          va_list  *app)
{
  if (steal)
    return va_arg (*app, GVariant *);
  else
    return g_variant_ref (va_arg (*app, GVariant *));
}

static GVariant *
g_variant_valist_new (gboolean    steal,
                      GSignature  signature,
                      va_list    *app)
{
  switch (g_signature_type (signature))
  {
    case G_SIGNATURE_TYPE_BOOLEAN:
      {
        guint8 byte = !!va_arg (*app, gboolean);
        return g_variant_new_small (signature, &byte, 1);
      }

    case G_SIGNATURE_TYPE_BYTE:
      {
        guint8 byte = va_arg (*app, guint);
        return g_variant_new_small (signature, &byte, 1);
      }

    case G_SIGNATURE_TYPE_INT16:
    case G_SIGNATURE_TYPE_UINT16:
      {
        guint16 integer_16 = va_arg (*app, guint);
        return g_variant_new_small (signature, &integer_16, 2);
      }

    case G_SIGNATURE_TYPE_INT32:
    case G_SIGNATURE_TYPE_UINT32:
      {
        guint32 integer_32 = va_arg (*app, guint);
        return g_variant_new_small (signature, &integer_32, 4);
      }

    case G_SIGNATURE_TYPE_INT64:
    case G_SIGNATURE_TYPE_UINT64:
      {
        guint64 integer_64 = va_arg (*app, guint64);
        return g_variant_new_small (signature, &integer_64, 8);
      }

    case G_SIGNATURE_TYPE_DOUBLE:
      {
        double floating = va_arg (*app, double);
        return g_variant_new_small (signature, &floating, 8);
      }

    case G_SIGNATURE_TYPE_STRING:
      return g_variant_new_string (va_arg (*app, const char *));

    case G_SIGNATURE_TYPE_OBJECT_PATH:
      return g_variant_new_object_path (va_arg (*app, const char *));

    case G_SIGNATURE_TYPE_SIGNATURE:
      return g_variant_new_signature (va_arg (*app, const char *));

    case G_SIGNATURE_TYPE_ANY:
      return g_variant_valist_get_ptr (steal, app);

    case G_SIGNATURE_TYPE_VARIANT:
      return g_variant_new_variant (g_variant_valist_get_ptr (steal, app));

    case G_SIGNATURE_TYPE_ARRAY:
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
            children[i] = g_variant_valist_new (steal, elemsig, app);
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

    case G_SIGNATURE_TYPE_STRUCT:
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
            children[i] = g_variant_valist_new (steal, itemsig, app);
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

    case G_SIGNATURE_TYPE_DICT_ENTRY:
      {
        GSVHelper *helper;
        GVariant **children;

        children = g_slice_alloc (sizeof (GVariant *) * 2);
        children[0] = g_variant_valist_new (steal,
                                            g_signature_key (signature),
                                            app);
        children[1] = g_variant_valist_new (steal,
                                            g_signature_value (signature),
                                            app);

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
g_variant_vvnew (gboolean    steal,
                 GSignature  signature,
                 va_list    *app)
{
  GVariant *value;

  value = g_variant_valist_new (steal, signature, app);
  g_variant_flatten (value);

  return value;
}

GVariant *
g_variant_vnew (gboolean   steal,
                GSignature signature,
                va_list    ap)
{
  GVariant *value;

  value = g_variant_valist_new (steal, signature, &ap);
  g_variant_flatten (value);
  return value;
}

GVariant *
g_variant_new_full (gboolean   steal,
                    GSignature signature,
                    ...)
{
  GVariant *value;
  va_list ap;

  va_start (ap, signature);
  value = g_variant_valist_new (steal, signature, &ap);
  va_end (ap);

  g_variant_flatten (value);

  return value;
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
  value = g_variant_valist_new (TRUE, signature, &ap);
  va_end (ap);

  g_variant_flatten (value);

  return value;
}
