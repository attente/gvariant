/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

/**
 * SECTION: gvariant
 * @title: GVariant
 * @short_description: a general purpose variant datatype
 * @see_also: GVariantType
 *
 * #GVariant is a variant datatype; it stores a value along with
 * information about the type of that value.  The range of possible
 * values is determined by the type.  The range of possible types is
 * exactly those types that may be sent over DBus.
 *
 * #GVariant instances always have a type and a value (which are given
 * at construction time).  The type and value of a #GVariant instance
 * can never change other than by the #GVariant itself being
 * destroyed.  A #GVariant can not contain a pointer.
 *
 * Facilities exist for serialising the value of a #GVariant into a
 * byte sequence.  A #GVariant can be sent over the bus or be saved to
 * disk.  Additionally, #GVariant is used as the basis of the
 * #GSettings persistent storage system.
 **/

#include "gvariant-serialiser.h"
#include "gvariant-private.h"

#include <string.h>
#include <glib.h>

/**
 * GVariant:
 *
 * #GVariant is an opaque data structure and can only be accessed
 * using the following functions.
 **/
struct OPAQUE_TYPE__GVariant
{
  union
  {
    struct
    {
      GVariant *source;
      guint8 *data;
    } serialised;

    struct
    {
      GVariant **children;
      gsize n_children;
    } tree;

    struct
    {
      GDestroyNotify callback;
      gpointer user_data;
    } notify;
  } contents;

  gsize size;
  GVariantTypeInfo *type;
  gboolean floating;
  GStaticMutex lock;
  guint state;
  gint ref_count;
};

#define check g_variant_assert_invariant

#define STATE_NATIVE            0x01
#define STATE_TRUSTED           0x02
#define STATE_SERIALISED        0x04
#define STATE_SIZE_KNOWN        0x08
#define STATE_RENORMALISED      0x10
#define STATE_FIXED_SIZE        0x20
#define STATE_INDEPENDENT       0x40
#define STATE_VISIBLE           0x80
#define STATE_SOURCE_NATIVE     0x100
#define STATE_NOTIFY            0x200
#define STATE_ZERO              0x400
#define STATE_LOCKED            0x80000000

static void g_variant_fill_gvs (GVariantSerialised *, gpointer);

static void
g_variant_lock (GVariant *value)
{
  g_static_mutex_lock (&value->lock);
}

static void
g_variant_unlock (GVariant *value)
{
  g_static_mutex_unlock (&value->lock);
}

static gboolean
g_variant_transition_size_known (GVariant *value)
{
  GVariant **children;
  gsize n_children;

  children = value->contents.tree.children;
  n_children = value->contents.tree.n_children;
  value->size = g_variant_serialiser_needed_size (value->type,
                                                  &g_variant_fill_gvs,
                                                  (gpointer *) children,
                                                  n_children);

  return TRUE;
}

static gboolean
g_variant_transition_serialised (GVariant *value)
{
  GVariantSerialised gvs;
  GVariant **children;
  gsize n_children;

  children = value->contents.tree.children;
  n_children = value->contents.tree.n_children;

  gvs.type = value->type;
  gvs.size = value->size;
  gvs.data = g_slice_alloc (gvs.size);

  g_variant_serialiser_serialise (gvs, &g_variant_fill_gvs,
                                  (gpointer *) children, n_children);

  value->contents.serialised.source = NULL;
  value->contents.serialised.data = gvs.data;

  return TRUE;
}

static gboolean
g_variant_transition_native (GVariant *value)
{
  GVariantSerialised gvs;

  gvs.type = value->type;
  gvs.size = value->size;
  gvs.data = value->contents.serialised.data;

  g_variant_serialised_byteswap (gvs);

  return TRUE;
}

static gboolean
g_variant_transition_trusted (GVariant *value)
{
  GVariantSerialised gvs;

  if ((value->state & STATE_INDEPENDENT) == 0)
    if (value->contents.serialised.source->state & STATE_TRUSTED)
      return TRUE;

  gvs.type = value->type;
  gvs.data = value->contents.serialised.data;
  gvs.size = value->size;

  return g_variant_serialised_is_normal (gvs);
}

static GVariant *
g_variant_deep_copy (GVariant *value)
{
  switch (g_variant_get_type_class (value))
  {
    case G_VARIANT_TYPE_CLASS_BOOLEAN:
      return g_variant_new_boolean (g_variant_get_boolean (value));

    case G_VARIANT_TYPE_CLASS_BYTE:
      return g_variant_new_byte (g_variant_get_byte (value));

    case G_VARIANT_TYPE_CLASS_INT16:
      return g_variant_new_int16 (g_variant_get_int16 (value));

    case G_VARIANT_TYPE_CLASS_UINT16:
      return g_variant_new_uint16 (g_variant_get_uint16 (value));

    case G_VARIANT_TYPE_CLASS_INT32:
      return g_variant_new_int32 (g_variant_get_int32 (value));

    case G_VARIANT_TYPE_CLASS_UINT32:
      return g_variant_new_uint32 (g_variant_get_uint32 (value));

    case G_VARIANT_TYPE_CLASS_INT64:
      return g_variant_new_int64 (g_variant_get_int64 (value));

    case G_VARIANT_TYPE_CLASS_UINT64:
      return g_variant_new_uint64 (g_variant_get_uint64 (value));

    case G_VARIANT_TYPE_CLASS_DOUBLE:
      return g_variant_new_double (g_variant_get_double (value));
      
    case G_VARIANT_TYPE_CLASS_STRING:
      return g_variant_new_string (g_variant_get_string (value, NULL));

    case G_VARIANT_TYPE_CLASS_OBJECT_PATH:
      return g_variant_new_object_path (g_variant_get_string (value, NULL));

    case G_VARIANT_TYPE_CLASS_SIGNATURE:
      return g_variant_new_signature (g_variant_get_string (value, NULL));

    case G_VARIANT_TYPE_CLASS_VARIANT:
      {
        GVariant *inside, *new;
       
        inside = g_variant_get_variant (value);
        new = g_variant_new_variant (g_variant_deep_copy (inside));
        g_variant_unref (inside);

        return new;
      }

    case G_VARIANT_TYPE_CLASS_MAYBE:
    case G_VARIANT_TYPE_CLASS_ARRAY:
    case G_VARIANT_TYPE_CLASS_STRUCT:
    case G_VARIANT_TYPE_CLASS_DICT_ENTRY:
      {
        GVariantBuilder *builder;
        GVariantIter iter;
        GVariant *child;

        builder = g_variant_builder_new (g_variant_get_type_class (value),
                                         g_variant_get_type (value));
        g_variant_iter_init (&iter, value);

        while ((child = g_variant_iter_next (&iter)))
          g_variant_builder_add_value (builder, g_variant_deep_copy (child));

        return g_variant_builder_end (builder);
      }

    default:
      g_assert_not_reached ();
  }
}

static gboolean
g_variant_transition_renormalised (GVariant *value)
{
  GVariant tmp;

  tmp = *value;
  g_static_mutex_init (&tmp.lock);
  value->contents.serialised.source = g_variant_deep_copy (&tmp);

  return TRUE;
}

static gboolean
g_variant_transition_independent (GVariant *value)
{
  GVariant *source;
  gpointer  new;

  new = g_slice_alloc (value->size);

  if (!(source->state & STATE_NATIVE))
    {
      memcpy (new, value->contents.serialised.data, value->size);

      /* barrier to ensure byteswap is not in progress */
      g_variant_lock (source);
      g_variant_unlock (source);
    }

  if (source->state & STATE_NATIVE)
    {
      memcpy (new, value->contents.serialised.data, value->size);
      value->state |= STATE_NATIVE;
    }

  value->contents.serialised.data = new;
  g_variant_unref (value->contents.serialised.source);
  value->contents.serialised.source = NULL; /* valgrind */

  return FALSE;
}

static gboolean
g_variant_transition_source_native (GVariant *value)
{
  return (value->contents.serialised.source->state & STATE_NATIVE) != 0;
}

struct transition
{
  guint required_states;
  guint forbidden_states;
};

struct state
{
  guint    state;
  void     (*assert_invariant) (GVariant *);
  gboolean (*enable) (GVariant *);
  struct transition transitions[4];
};

struct state state_table[] =
{
  { STATE_NATIVE, NULL, g_variant_transition_native,
    { { STATE_SOURCE_NATIVE                                     },
      { STATE_SERIALISED | STATE_INDEPENDENT | STATE_FIXED_SIZE },
      { STATE_SERIALISED | STATE_INDEPENDENT | STATE_TRUSTED    },
      { STATE_LOCKED                                            } } },

  { STATE_TRUSTED, NULL, g_variant_transition_trusted,
    { { STATE_SERIALISED, STATE_RENORMALISED                    },
      { STATE_LOCKED                                            } } },

  { STATE_SERIALISED, NULL, g_variant_transition_serialised,
    { { STATE_SIZE_KNOWN                                        },
      { STATE_LOCKED                                            } } },

  { STATE_SIZE_KNOWN, NULL, g_variant_transition_size_known,
    { {                                                         },
      { STATE_LOCKED                                            } } },

  { STATE_RENORMALISED, NULL, g_variant_transition_renormalised,
    { { STATE_SERIALISED | STATE_INDEPENDENT,
        STATE_NATIVE | STATE_FIXED_SIZE | STATE_TRUSTED         },
      { STATE_LOCKED                                            } } },

  { STATE_FIXED_SIZE, NULL, NULL,
    {
      { STATE_LOCKED                                            } } },

  { STATE_INDEPENDENT, NULL, g_variant_transition_independent,
    { { STATE_SERIALISED,
        STATE_NATIVE | STATE_RENORMALISED | STATE_VISIBLE       },
      { STATE_LOCKED                                            } } },

  { STATE_VISIBLE, NULL, NULL,
    { { STATE_NATIVE | STATE_SERIALISED                         },
      { STATE_RENORMALISED                                      },
      { STATE_LOCKED                                            } } },

  { STATE_SOURCE_NATIVE, NULL, g_variant_transition_source_native,
    { { STATE_SERIALISED, STATE_INDEPENDENT                     },
      { STATE_LOCKED                                            } } },

  { STATE_NOTIFY, NULL, NULL,
    { { STATE_LOCKED                                            } } }
};

static gboolean
g_variant_try_state (GVariant *value,
                     guint     state)
{
  if ((value->state & state) == state)
    return TRUE;
  
  g_variant_lock (value);

  if ((value->state & state) != state)
    {
      gsize i;

      for (i = 0; i < G_N_ELEMENTS (state_table); i++)
        if ((state & state_table[i].state) >
            (state_table[i].state & value->state))
          {
            struct state *s = &state_table[i];
            struct transition *t;

            /* see if we can transition directly first */
            for (t = s->transitions; t->required_states != STATE_LOCKED; t++)
              if ((t->forbidden_states & value->state) == 0 &&
                  (t->required_states & value->state) == t->required_states)
                if (s->enable == NULL || s->enable (value))
                  {
                    value->state |= s->state;
                    goto ok;
                  }

            /* try to request other states to satisfy our prereqs */
            for (t = s->transitions; t->required_states != STATE_LOCKED; t++)
              if ((t->forbidden_states & value->state) == 0 &&
                  g_variant_try_state (value, t->required_states))
              {
                /* one of the other states may have given us this one
                 * for free already.  double-check that.
                 */
                if (value->state & s->state)
                  goto ok;

                else if (s->enable == NULL || s->enable (value))
                  {
                    value->state |= s->state;
                    goto ok;
                  }
              }

            /* failed to get this particular state */
            g_variant_unlock (value);
            return FALSE;

            ok: continue;
          }
    }

  g_variant_unlock (value);

  g_assert ((value->state & state) == state);

  return TRUE;
}

static void
g_variant_require_state (GVariant *value,
                         guint     state)
{
  gboolean success;

  success = g_variant_try_state (value, state);
  g_assert (success);
}

static gboolean
g_variant_is_out_of_state (GVariant *value,
                           guint     state)
{
  if (value->state & state)
    return FALSE;

  g_variant_lock (value);

  if (value->state & state)
    {
      g_variant_unlock (value);
      return FALSE;
    }

  return TRUE;
}













/* this is the only function that ever allocates a new GVariant structure.
 * g_variant_unref() is the only function that ever frees one.
 */
static GVariant *
g_variant_alloc (GVariantTypeInfo *type,
                 guint             initial_state)
{
  GVariant *variant;

  variant = g_slice_new (GVariant);
  variant->ref_count = 1;
  variant->type = type;
  variant->floating = TRUE;
  variant->state = initial_state;
  g_static_mutex_init (&variant->lock);

  return variant;
}

static GVariantSerialised
g_variant_get_gvs (GVariant  *value,
                   GVariant **source)
{
  GVariantSerialised gvs = { value->type };

  g_assert (value->state & STATE_SERIALISED);

  /* not independent implies not renormalised */
  if (g_variant_is_out_of_state (value, STATE_INDEPENDENT))
    {
      /* dependent */
      gvs.data = value->contents.serialised.data;
      gvs.size = value->size;

      if (source)
        *source = g_variant_ref (value->contents.serialised.source);

      g_variant_unlock (value);
    }
  else if (g_variant_is_out_of_state (value, STATE_RENORMALISED))
    {
      /* independent */
      gvs.data = value->contents.serialised.data;
      gvs.size = value->size;

      if (source)
        *source = g_variant_ref (value);

      g_variant_unlock (value);
    }
  else
    {
      /* renormalised */
      gvs.data = value->contents.serialised.source->contents.serialised.data;
      gvs.size = value->contents.serialised.source->size;

      if (source)
        *source = g_variant_ref (value->contents.serialised.source);
    }

  return gvs;
}

/*
 * g_variant_fill_gvs:
 * @serialised: the #GVariantSerialised to fill
 * @data: our #GVariant instance
 *
 * Utility function used as a callback from the serialiser to get
 * information about a given #GVariant instance (in @data).
 */
static void
g_variant_fill_gvs (GVariantSerialised *serialised,
                    gpointer            data)
{
  GVariant *value = data;

  check (value);
  g_variant_require_state (value, STATE_SIZE_KNOWN);

  if (serialised->type == NULL)
    serialised->type = value->type;

  if (serialised->size == 0)
    serialised->size = value->size;

  g_assert (serialised->type == value->type);
  g_assert (serialised->size == value->size);

  if (serialised->data && serialised->size)
    g_variant_store (value, serialised->data);
}

static guchar *
g_variant_get_zeros (gsize size)
{
  static guchar zeros[4096];

  g_assert (size <= 4096);

  return zeros;
}

static GVariant *
g_variant_from_gvs (GVariantSerialised  gvs,
                    GVariant           *source,
                    gboolean            trusted)
{
  GVariant *new;

  g_assert (source->state & STATE_INDEPENDENT);

  new = g_variant_alloc (gvs.type, STATE_SERIALISED | STATE_SIZE_KNOWN);

  if (gvs.data == NULL)
    {
      if (gvs.size)
        {
          g_assert (!(source->state & STATE_TRUSTED));
          g_assert (!trusted);

          new->contents.serialised.data = g_variant_get_zeros (gvs.size);
        }
      else
        new->contents.serialised.data = NULL;

      new->contents.serialised.source = NULL;
      new->size = gvs.size;

      new->state |= STATE_INDEPENDENT | STATE_NATIVE | STATE_ZERO;

      if (gvs.size)
        new->state |= STATE_TRUSTED;
    }
  else
    {
      new->contents.serialised.source = g_variant_ref (source);
      new->contents.serialised.data = gvs.data;
      new->size = gvs.size;

      if (source->state & STATE_NATIVE)
        new->state |= STATE_NATIVE;

      if (trusted || source->state & STATE_TRUSTED)
        new->state |= STATE_TRUSTED;
    }

  check (new);

  return new;
}

/**
 * g_variant_get_child:
 * @value: a container #GVariant
 * @index: the index of the child to fetch
 * @returns: the child at the specified index
 *
 * Reads a child item out of a container #GVariant instance.  This
 * includes variants, maybes, arrays, structures and dictionary
 * entries.  It is an error to call this function on any other type of
 * #GVariant.
 *
 * It is an error if @index is greater than the number of child items
 * in the container.  See g_variant_get_n_children().
 *
 * This function never fails.
 **/
GVariant *
g_variant_get_child (GVariant *value,
                     gsize     index)
{
  GVariant *child;

  check (value);

  if (g_variant_is_out_of_state (value, STATE_SERIALISED))
    {
      if G_UNLIKELY (index >= value->contents.tree.n_children)
        g_error ("Attempt to access item %d in a container with "
                 "only %d items", index, value->contents.tree.n_children);

      child = g_variant_ref (value->contents.tree.children[index]);
      g_variant_unlock (value);
    }
  else
    {
      GVariantSerialised gvs;
      GVariant *source;

      gvs = g_variant_get_gvs (value, &source);
      gvs = g_variant_serialised_get_child (gvs, index);
      child = g_variant_from_gvs (gvs, source,
                                  value->state & STATE_TRUSTED);
      g_variant_unref (source);
    }

  check (child);

  return child;
}

/**
 * g_variant_n_children:
 * @value: a container #GVariant
 * @returns: the number of children in the container
 *
 * Determines the number of children in a container #GVariant
 * instance.  This includes variants, maybes, arrays, structures and
 * dictionary entries.  It is an error to call this function on any
 * other type of #GVariant.
 *
 * For variants, the return value is always 1.  For maybes, it is
 * always zero or one.  For arrays, it is the length of the array.
 * For structures it is the number of structure items (which depends
 * only on the type).  For dictionary entries, it is always 2.
 *
 * This function never fails.
 * TS
 **/
gsize
g_variant_n_children (GVariant *value)
{
  gsize n_children;

  check (value);

  if (g_variant_is_out_of_state (value, STATE_SERIALISED))
    {
      n_children = value->contents.tree.n_children;
      g_variant_unlock (value);
    }
  else
    {
      GVariantSerialised gvs;
      GVariant *source;

      gvs = g_variant_get_gvs (value, &source);
      n_children = g_variant_serialised_n_children (gvs);
      g_variant_unref (source);
    }

  return n_children;
}

gsize
g_variant_get_size (GVariant *value)
{
  g_variant_require_state (value, STATE_VISIBLE);
  return g_variant_get_gvs (value, NULL).size;
}

gconstpointer
g_variant_get_data (GVariant *value)
{
  g_variant_require_state (value, STATE_VISIBLE);
  return g_variant_get_gvs (value, NULL).data;
}

/**
 * g_variant_store:
 * @value: the #GVariant to store
 * @data: the location to store the serialised data at
 *
 * Stores the serialised form of @variant at @data.  @data should be
 * serialised enough.  See g_variant_get_size().
 *
 * The stored data is in machine native byte order but may not be in
 * fully-normalised form if read from an untrusted source.  See
 * g_variant_normalise() for a solution.
 *
 * This function is approximately O(n) in the size of @data.
 *
 * This function never fails.
 **/
void
g_variant_store (GVariant *value,
                 gpointer  data)
{
  check (value);

  g_variant_require_state (value, STATE_SIZE_KNOWN | STATE_NATIVE);

  if (g_variant_is_out_of_state (value, STATE_SERIALISED))
    {
      GVariantSerialised gvs;
      GVariant **children;
      gsize n_children;

      gvs.type = value->type;
      gvs.data = data;
      gvs.size = value->size;

      children = value->contents.tree.children;
      n_children = value->contents.tree.n_children;

      g_variant_serialiser_serialise (gvs,
                                      &g_variant_fill_gvs,
                                      (gpointer *) children,
                                      n_children);
    }
  else
    {
      GVariantSerialised gvs;
      GVariant *source;

      gvs = g_variant_get_gvs (value, &source);
      memcpy (data, gvs.data, gvs.size);
      g_variant_unref (source);
  }
}

/**
 * g_variant_get_fixed:
 * @value: a #GVariant
 * @size: the size of @value
 * @returns: a pointer to the fixed-sized data
 *
 * Gets a pointer to the data of a fixed sized #GVariant instance.
 * This pointer can be treated as a pointer to the equivalent C
 * stucture type and accessed directly.  The data is in machine byte
 * order.
 *
 * @size must be equal to the fixed size of the type of @value.  It
 * isn't used for anything, but serves as a sanity check to ensure the
 * user of this function will be able to make sense of the data they
 * receive a pointer to.
 *
 * This function may return %NULL if @size is zero.
 **/
gconstpointer
g_variant_get_fixed (GVariant *value,
                     gsize     size)
{
  gsize fixed_size;

  g_variant_type_info_query (value->type, NULL, &fixed_size);
  g_assert (fixed_size);

  g_assert_cmpint (size, ==, fixed_size);

  return g_variant_get_data (value);
}

/**
 * g_variant_get_fixed_array:
 * @value: an array #GVariant
 * @elem_size: the size of one array element
 * @length: a pointer to the length of the array, or %NULL
 * @returns: a pointer to the array data
 *
 * Gets a pointer to the data of an array of fixed sized #GVariant
 * instances.  This pointer can be treated as a pointer to an array of
 * the equivalent C structure type and accessed directly.  The data is
 * in machine byte order.
 *
 * @elem_size must be equal to the fixed size of the element type of
 * @value.  It isn't used for anything, but serves as a sanity check
 * to ensure the user of this function will be able to make sense of
 * the data they receive a pointer to.
 *
 * @length is set equal to the number of items in the array, so that
 * the size of the memory region returned is @elem_size times @length.
 *
 * This function may return %NULL if either @elem_size or @length is
 * zero.
 */
gconstpointer
g_variant_get_fixed_array (GVariant *value,
                           gsize     elem_size,
                           gsize    *length)
{
  gsize fixed_elem_size;

  /* unsupported: maybes are treated as arrays of size zero or one */
  g_variant_type_info_query_element (value->type, NULL, &fixed_elem_size);
  g_assert (fixed_elem_size);

  g_assert_cmpint (elem_size, ==, fixed_elem_size);

  if (length != NULL)
    *length = g_variant_n_children (value);

  return g_variant_get_data (value);
}

/*
 * g_variant_apply_flags:
 * @value: a fresh #GVariant instance
 * @flags: various load flags
 *
 * This function is the common code used to apply flags (normalise,
 * byteswap, etc) to fresh #GVariant instances created using one of
 * the load functions.
 */
static GVariant *
g_variant_apply_flags (GVariant      *value,
                       GVariantFlags  flags)
{
  guint16 byte_order = flags;

  if (byte_order == 0)
    byte_order = G_BYTE_ORDER;

  g_assert (byte_order == G_LITTLE_ENDIAN ||
            byte_order == G_BIG_ENDIAN);

  if (byte_order == G_BYTE_ORDER)
    value->state |= STATE_NATIVE;

  else if (!flags & G_VARIANT_LAZY_BYTESWAP)
    g_variant_require_state (value, STATE_NATIVE);

  check (value);

  return value;
}

/**
 * g_variant_unref:
 * @value: a #GVariant
 *
 * Decreases the reference count of @variant.  When its reference
 * count drops to 0, the memory used by the variant is freed.
 **/
void
g_variant_unref (GVariant *value)
{
  check (value);

  if (g_atomic_int_dec_and_test (&value->ref_count))
    {
      /* free the type info */
      if (value->type)
        g_variant_type_info_unref (value->type);

      /* free the data */
      if (value->state & STATE_SERIALISED)
        {
          if (value->state & STATE_RENORMALISED ||
              !(value->state & STATE_INDEPENDENT))
            g_variant_unref (value->contents.serialised.source);

          if (value->state & STATE_INDEPENDENT &&
              !(value->state & STATE_ZERO))
            g_slice_free1 (value->size, value->contents.serialised.data);
        }
      else
        {
          GVariant **children;
          gsize n_children;
          gsize i;

          children = value->contents.tree.children;
          n_children = value->contents.tree.n_children;

          for (i = 0; i < n_children; i++)
            g_variant_unref (children[i]);

          g_slice_free1 (sizeof (GVariant *) * n_children, children);
        }

      /* free the structure itself */
      g_slice_free (GVariant, value);
    }
}

/**
 * g_variant_ref:
 * @value: a #GVariant
 * @returns: the same @variant
 *
 * Increases the reference count of @variant.
 **/
GVariant *
g_variant_ref (GVariant *value)
{
  check (value);

  g_atomic_int_inc (&value->ref_count);

  return value;
}

/**
 * g_variant_ref_sink:
 * @value: a #GVariant
 * @returns: the same @variant
 *
 * If @value is floating, mark it as no longer floating.  If it is not
 * floating, increase its reference count.
 **/
GVariant *
g_variant_ref_sink (GVariant *value)
{
  check (value);

  g_variant_ref (value);
  if (g_atomic_int_compare_and_exchange (&value->floating, 1, 0))
    g_variant_unref (value);

  return value;
}

GVariant *
g_variant_ensure_floating (GVariant *value)
{
  check (value);

  g_variant_ref_sink (value);

  if (value->ref_count == 1)
    /* it is exclusively ours */
    {
      value->floating = TRUE;

      return value;
    }
  else
    /* someone else has it too.  make our own. */
    {
      GVariant *new;

      g_error ("not yet supported");
      new = NULL;
      g_variant_unref (value);

      return new;
    }
}

/* private */
GVariant *
g_variant_new_tree (const GVariantType  *type,
                    GVariant           **children,
                    gsize                n_children,
                    gboolean             trusted)
{
  GVariant *new;

  new = g_variant_alloc (g_variant_type_info_get (type),
                         STATE_INDEPENDENT | STATE_NATIVE);

  new->contents.tree.children = children;
  new->contents.tree.n_children = n_children;
  new->size = 0;

  if (trusted)
    new->state |= STATE_TRUSTED;

  check (new);

  return new;
}

/**
 * g_variant_from_slice:
 * @type: the #GVariantType of the new variant
 * @slice: a pointer to a GSlice-allocated region
 * @size: the size of @slice
 * @flags: zero or more #GVariantLoadFlags
 * @returns: a new #GVariant instance
 *
 * Creates a #GVariant instance from a memory slice.  Ownership of the
 * memory slice is assumed.  This function allows efficiently creating
 * #GVariant instances with data that is, for example, read over a
 * socket.
 *
 * This function never fails.
 **/
GVariant *
g_variant_from_slice (const GVariantType *type,
                      gpointer            slice,
                      gsize               size,
                      GVariantFlags       flags)
{
  GVariant *new;

  new = g_variant_alloc (g_variant_type_info_get (type),
                         STATE_SERIALISED | STATE_INDEPENDENT |
                         STATE_SIZE_KNOWN);

  new->contents.serialised.source = NULL;
  new->contents.serialised.data = slice;
  new->size = size;

  return g_variant_apply_flags (new, flags);
}

GVariant *
g_variant_from_data (const GVariantType *type,
                     gconstpointer       data,
                     gsize               size,
                     GVariantFlags       flags,
                     GDestroyNotify      notify,
                     gpointer            user_data)
{
  GVariant *new;

  if (type == NULL)
    {
      GVariant *variant;

      variant = g_variant_from_data (G_VARIANT_TYPE_VARIANT,
                                     data, size, flags, notify, user_data);
      new = g_variant_get_variant (variant);
      g_variant_unref (variant);

      return new;
    }
  else
    {
      GVariant *marker;

      marker = g_variant_alloc (NULL, STATE_NOTIFY);
      marker->contents.notify.callback = notify;
      marker->contents.notify.user_data = user_data;

      new = g_variant_alloc (g_variant_type_info_get (type),
                             STATE_SERIALISED | STATE_SIZE_KNOWN);
      new->contents.serialised.source = marker;
      new->contents.serialised.data = (gpointer) data;
      new->size = size;

      return g_variant_apply_flags (new, flags);
    }
}

GVariant *
g_variant_load (const GVariantType *type,
                gconstpointer       data,
                gsize               size,
                GVariantFlags       flags)
{
  GVariant *new;

  if (type == NULL)
    {
      GVariant *variant;

      variant = g_variant_load (G_VARIANT_TYPE_VARIANT, data, size, flags);
      new = g_variant_get_variant (variant);
      g_variant_unref (variant);

      return new;
    }
  else
    {
      gpointer slice;

      slice = g_slice_alloc (size);
      memcpy (slice, data, size);

      return g_variant_from_slice (type, slice, size, flags);
    }
}

/**
 * g_variant_get_type:
 * @value: a #GVariant
 * @returns: a #GVariantType
 *
 * Determines the type of @value.
 *
 * The return value is valid for the lifetime of @value and must not
 * be freed.
 */
const GVariantType *
g_variant_get_type (GVariant *value)
{
  check (value);

  return g_variant_type_info_get_type (value->type);
}

/**
 * g_variant_load:
 * @type: the #GVariantType of the new variant
 * @data: the serialised #GVariant data to load
 * @size: the size of @data
 * @flags: zero or more #GVariantLoadFlags
 * @returns: a new #GVariant instance
 *
 * Creates a new #GVariant instance.  @data is copied.  For a more
 * efficient way to create #GVariant instances, see
 * g_variant_from_slice() or XXX.
 *
 * This function is O(n) in the size of @data.
 *
 * This function never fails.
 **/

/*
 * g_variant_assert_invariant:
 * @value: a #GVariant instance to check
 *
 * This function asserts the class invariant on a #GVariant instance.
 * Any detected problems will result in an assertion failure.
 *
 * This function is potentially very slow.
 *
 * This function never fails.
 * TS
 */
void
g_variant_assert_invariant (GVariant *value)
{
#if 0
  GVariantSerialised gvs;

  g_assert (value != NULL);

  g_variant_lock (value);

  g_assert_cmpint (value->ref_count, >, 0);
  g_assert (value->type != NULL);

  switch (value->representation)
  {
    case G_VARIANT_TREE:
      g_variant_unlock (value);
      return;

    case G_VARIANT_SERIALISED:
      if (value->contents.serialised.source)
        {
          GVariant *source = value->contents.serialised.source;

          g_assert (source->native_endian || !value->native_endian);
          g_assert (source->representation == G_VARIANT_SERIALISED ||
                    source->representation == G_VARIANT_NOTIFY);

          if (source->representation == G_VARIANT_SERIALISED)
            {
              /* no grandparents */
              g_assert (source->contents.serialised.source == NULL);
              g_variant_assert_invariant (source);
            }
        }

      gvs.type = value->type;
      gvs.size = value->size;
      gvs.data = value->contents.serialised.data;
      break;

    case G_VARIANT_RENORMALISED:
      g_assert (value->trusted == FALSE);
      g_assert (value->contents.serialised.source != NULL);
      g_assert (value->contents.serialised.source->representation ==
                G_VARIANT_SERIALISED);
      g_variant_assert_invariant (value->contents.serialised.source);

      gvs.type = value->type;
      gvs.size = value->size;
      gvs.data = value->contents.serialised.data;

    default:
      g_assert_not_reached ();
  }

  g_variant_serialised_assert_invariant (gvs);
  g_variant_unlock (value);
#endif
}

gboolean
g_variant_is_trusted (GVariant *value)
{
  return !!(value->state & STATE_TRUSTED);
}
