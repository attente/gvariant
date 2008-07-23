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
 * @see_also: gsignature
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
 * byte stream.  A #GVariant can be sent over the bus or be saved to
 * disk.  Additionally, #GVariant is used as the basis of the
 * #GSettings persistent storage system.
 **/

#include "gvariant-private.h"
#include "gsvariant.h"

#include <string.h>
#include <glib.h>

typedef enum
{
  G_VARIANT_SMALL,
  G_VARIANT_LARGE,
  G_VARIANT_TREE,
  G_VARIANT_NOTIFY
} GVariantRepresentation;

#define G_VARIANT_SMALL_SIZE 8

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
      guchar data[G_VARIANT_SMALL_SIZE];
      guchar size;
    } small;

    struct
    {
      GVariant *source;
      guint8 *data;
      gsize size;
    } large;

    struct
    {
      gsize serialised_size;

      GVariant **children;
      gsize n_children;
    } tree;

    struct
    {
      GDestroyNotify callback;
      gpointer user_data;
    } notify;
  } contents;

  GSVHelper *signature;

  guint locked : 1;

  guint representation : 2;
  guint trusted : 1;
  guint native_endian : 1;

  gint ref_count;
};

#define check g_variant_assert_invariant

static GStaticMutex g_variant_meta_lock = G_STATIC_MUTEX_INIT;
static GSList *g_variant_contention_list = NULL;
typedef struct
{
  GCond *condition;
  GVariant *value;
  gboolean clear;
} ContentionItem;

static void
g_variant_lock (GVariant *value)
{
  g_static_mutex_lock (&g_variant_meta_lock);

  if (G_UNLIKELY (value->locked))
    /* contended case */
    {
      ContentionItem *item;

      item = g_slice_new (ContentionItem);
      item->value = value;
      item->condition = g_cond_new ();
      item->clear = FALSE;

      g_variant_contention_list = g_slist_prepend (g_variant_contention_list,
                                                   item);

      while (!item->clear)
        {
          g_static_mutex_unlock (&g_variant_meta_lock);
          g_cond_wait (item->condition, NULL);
          g_static_mutex_lock (&g_variant_meta_lock);
        }

      g_variant_contention_list = g_slist_remove (g_variant_contention_list,
                                                  item);
      g_slice_free (ContentionItem, item);
    }
  else
    value->locked = TRUE;

  g_static_mutex_unlock (&g_variant_meta_lock);
}

static void
g_variant_unlock (GVariant *value)
{
  GSList *node;

  g_static_mutex_lock (&g_variant_meta_lock);

  /* the contention list is expected to be -extremely- short */
  for (node = g_variant_contention_list; node; node = node->next)
    {
      ContentionItem *item = node->data;

      if (item->value == value)
        {
          item->clear = TRUE;
          g_cond_signal (item->condition);

          goto unlock;
        }
    }

  value->locked = FALSE;

 unlock:
  g_static_mutex_unlock (&g_variant_meta_lock);
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
  /* why normalise if it's already trusted? */
  g_assert (!((flags & G_VARIANT_NORMALISE) &&
              (flags & G_VARIANT_TRUSTED)));

  if (flags & G_VARIANT_NORMALISE)
    {
      value->trusted = FALSE;
      //XXX value = g_variant_normalise (value);
      g_assert (value->trusted);
    }

  else if (flags & G_VARIANT_TRUSTED)
    value->trusted = TRUE;

  else
    value->trusted = FALSE;

  /* can't byteswap both now and later... */
  g_assert (!((flags & G_VARIANT_BYTESWAP_NOW) &&
              (flags & G_VARIANT_BYTESWAP_LAZY)));

  /* always byteswap the little ones */
  if (flags & G_VARIANT_BYTESWAP_NOW)
    {
      value->native_endian = FALSE;
      g_variant_ensure_native_endian (value);
      g_assert (value->native_endian);
    }

  else if (flags & G_VARIANT_BYTESWAP_LAZY)
    {
      if (value->representation == G_VARIANT_SMALL)
        {
          /* do it now */
          GSVariant gsv = { value->signature,
                            value->contents.small.data,
                            value->contents.small.size };
          g_svariant_byteswap (gsv);
          value->native_endian = TRUE;
        }
      else
        /* do it later */
        value->native_endian = FALSE;
    }

  else
    value->native_endian = TRUE;

  check (value);

  return value;
}

/**
 * g_variant_unref:
 * @value: a #GVariant
 * @returns: %NULL
 *
 * Decreases the reference count of @variant.  When its reference
 * count drops to 0, the memory used by the variant is freed.
 **/
GVariant *
g_variant_unref (GVariant *value)
{
  check (value);

  if (g_atomic_int_dec_and_test (&value->ref_count))
    {
      /* free the signature */
      g_svhelper_unref (value->signature);

      /* free the data */
      switch (value->representation)
      {
        case G_VARIANT_SMALL:
          break;

        case G_VARIANT_LARGE:
          if (value->contents.large.source)
            g_variant_unref (value->contents.large.source);
          else
            g_slice_free1 (value->contents.large.size,
                           value->contents.large.data);
          break;

        case G_VARIANT_TREE:
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
          break;

        case G_VARIANT_NOTIFY:
          {
            gpointer user_data;

            user_data = value->contents.notify.user_data;
            value->contents.notify.callback (user_data);

            break;
          }
      }

      /* free the structure itself */
      g_slice_free (GVariant, value);
    }

  return NULL;
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

/* this is the only function that ever allocates a new GVariant structure.
 * g_variant_unref() is the only function that ever frees one.
 */
static GVariant *
g_variant_alloc (GVariantRepresentation  representation,
                 GSVHelper              *signature)
{
  GVariant *variant;

  /* constant expression sanity checks.  will compile out. */
  g_assert ((G_STRUCT_OFFSET (GVariant, contents.small.data) & 7) == 0);
  g_assert (sizeof (GVariant) <= sizeof (gpointer) * 6);

  variant = g_slice_new (GVariant);
  variant->ref_count = 1;
  variant->representation = representation;
  variant->signature = signature;
  variant->locked = FALSE;

  return variant;
}

/* private */
GVariant *
g_variant_new_tree (GSVHelper  *helper,
                    GVariant  **children,
                    gsize       n_children,
                    gboolean    trusted)
{
  GVariant *variant;

  variant = g_variant_alloc (G_VARIANT_TREE, helper);
  variant->contents.tree.children = children;
  variant->contents.tree.n_children = n_children;
  variant->contents.tree.serialised_size = -1;
  variant->trusted = trusted;

  check (variant);

  return variant;
}

/* private */
GVariant *
g_variant_new_small (GSignature signature,
                     gpointer   data,
                     gsize      size)
{
  GVariant *value;

  g_assert (size <= G_VARIANT_SMALL_SIZE);

  value = g_variant_alloc (G_VARIANT_SMALL, g_svhelper_get (signature));
  memcpy (&value->contents.small.data, data, size);
  value->contents.small.size = size;
  value->native_endian = TRUE;
  value->trusted = TRUE;

  check (value);

  return value;
}

/**
 * g_variant_from_slice:
 * @signature: the #GSignature of the new variant
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
g_variant_from_slice (GSignature    signature,
                      gpointer      slice,
                      gsize         size,
                      GVariantFlags flags)
{
  GSVHelper *helper;
  GVariant *variant;

  helper = g_svhelper_get (signature);

  if (size <= G_VARIANT_SMALL_SIZE)
    {
      variant = g_variant_alloc (G_VARIANT_SMALL, helper);
      memcpy (variant->contents.small.data, slice, size);
      variant->contents.small.size = size;

      g_slice_free1 (size, slice);
    }
  else
    {
      variant = g_variant_alloc (G_VARIANT_LARGE, helper);
      variant->contents.large.source = NULL;
      variant->contents.large.data = slice;
      variant->contents.large.size = size;
    }

  return g_variant_apply_flags (variant, flags);
}

/* private */
void
g_variant_get_small (GVariant *variant,
                     gpointer  data,
                     gsize     size)
{
  check (variant);

  g_assert (variant->representation == G_VARIANT_SMALL);
  g_assert (variant->contents.small.size == size);
  g_assert (size <= G_VARIANT_SMALL_SIZE);

  memcpy (data, variant->contents.small.data, size);
}

/*
 * g_variant_copy_safely:
 * @value: the value that 'owns' the data
 * @dest: the destination to copy to
 * @src: the source data (owned by @value)
 * @size: the number of bytes to copy
 * @returns: if the data in @dest is in native byte order
 *
 * Carefully copies the contents of @src to @dest and returns if the
 * bytes that were copied are in native byte order.
 *
 * This function can deal with the rare case that the byte order of
 * @value was changed during the copy operation.
 */
static gboolean
g_variant_copy_safely (GVariant     *source,
                       guchar       *dest,
                       const guchar *src,
                       gsize         size)
{
  if (source->representation == G_VARIANT_SMALL)
    {
      g_assert (source->native_endian);
      memcpy (dest, src, size);
      return TRUE;
    }

  g_assert (source->representation == G_VARIANT_LARGE ||
            source->representation == G_VARIANT_NOTIFY);
  g_assert (source->contents.large.source == NULL);

  if (!((volatile GVariant *)source)->native_endian)
    {
      memcpy (dest, src, size);

      /* barrier to ensure byteswap is not in progress */
      g_variant_lock (source);
      g_variant_unlock (source);
    }

  if (((volatile GVariant *)source)->native_endian)
    {
      memcpy (dest, src, size);
      return TRUE;
    }

  return FALSE;
}

/*
 * g_variant_get_gsv:
 * @value: the #GVariant
 * @source: a pointer to set to the owner of the return value
 * @returns: a #GSVariant that is valid until @source is unref'ed
 *
 * Obtains the #GSVariant data of a serialised #GVariant instance.
 * The return value will be valid until the pointer stored at @source
 * is unreffed.
 */
static GSVariant
g_variant_get_gsv (GVariant  *value,
                   GVariant **source)
{
  GSVariant gsv = { value->signature };

  check (value);

  switch (value->representation)
  {
    case G_VARIANT_SMALL:
      if (source)
        *source = g_variant_ref (value);

      gsv.size = value->contents.small.size;
      gsv.data = gsv.size ? value->contents.small.data : NULL;
      break;

    case G_VARIANT_LARGE:
      if (source && value->contents.large.source)
        {
          /* it is possible that someone may unparent this value at
           * any time we are unlocked, so be careful here.
           */

          g_variant_lock (value);

          if (value->contents.large.source)
            *source = g_variant_ref (value->contents.large.source);
          else
            *source = g_variant_ref (value);
          gsv.data = value->contents.large.data;

          g_variant_unlock (value);
        }
      else
        {
          gsv.data = value->contents.large.data;

          if (source)
            *source = g_variant_ref (value);
        }

      gsv.size = value->contents.large.size;
      break;

    default:
      g_assert_not_reached ();
  }

  return gsv;
}

static GVariant *
g_variant_from_gsv (GSVariant  gsv,
                    GVariant  *source,
                    gboolean   trusted)
{
  GVariant *value;

  if (gsv.size <= G_VARIANT_SMALL_SIZE)
    {
      value = g_variant_alloc (G_VARIANT_SMALL, g_svhelper_ref (gsv.helper));
      value->contents.small.size = gsv.size;

      if (gsv.size)
        {
          gboolean native;

          native = g_variant_copy_safely (source,
                                          value->contents.small.data,
                                          gsv.data, gsv.size);
          if (!native)
            {
              /* all SMALL variants are native-endian */
              gsv.data = value->contents.small.data;
              g_svariant_byteswap (gsv);
            }
        }

      value->native_endian = TRUE;
    }
  else
    {
      g_assert (source->representation == G_VARIANT_LARGE ||
                source->representation == G_VARIANT_NOTIFY);

      value = g_variant_alloc (G_VARIANT_LARGE, g_svhelper_ref (gsv.helper));
      value->contents.large.source = g_variant_ref (source);
      value->contents.large.size = gsv.size;
      value->contents.large.data = gsv.data;
      value->native_endian = source->native_endian;
    }

  value->trusted = trusted || source->trusted;
  check (value);

  return value;
}

/**
 * g_variant_get_child:
 * @container: a container #GVariant
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
g_variant_get_child (GVariant *container,
                     gsize     index)
{
  GVariant *source;
  GVariant *child;
  GSVariant gsv;

  if (container->representation == G_VARIANT_TREE)
    {
      g_variant_lock (container);

      if (G_UNLIKELY (container->representation != G_VARIANT_TREE))
        {
          g_variant_unlock (container);
          goto not_a_tree;
        }

      if (G_LIKELY (index < container->contents.tree.n_children))
        {
          child = g_variant_ref (container->contents.tree.children[index]);
          g_variant_unlock (container);
          return child;
        }

      g_error ("Attempt to access item %d in a container with "
               "only %d items", index, container->contents.tree.n_children);
    }

 not_a_tree:
  gsv = g_variant_get_gsv (container, &source);
  gsv = g_svariant_get_child (gsv, index);

  if (gsv.data == NULL)
    /* error */
    {
      gssize size;

      /* if the item is fixed-size then we need to allocate some
       * zero-filled bytes for it since the user might ask to see the
       * pointer.  if not, then a size of zero is valid.
       */
      g_svhelper_info (gsv.helper, NULL, &size);

      g_svhelper_ref (gsv.helper);

      if (size <= G_VARIANT_SMALL_SIZE)
        {
          if (size < 0)
            size = 0;

          child = g_variant_alloc (G_VARIANT_SMALL, gsv.helper);
          child->trusted = size >= 0;
          if (child->trusted)
            {
              memset (child->contents.small.data, 0, size);
              child->contents.small.size = size;
            }
          else
            child->contents.small.size = 0;
          child->native_endian = TRUE;
        }
      else
        {
          child = g_variant_alloc (G_VARIANT_LARGE, gsv.helper);
          child->contents.large.source = NULL;
          child->contents.large.data = g_slice_alloc0 (size);
          child->contents.large.size = size;
        }
    }
  else
    /* no error */
    child = g_variant_from_gsv (gsv, source, container->trusted);

  g_svhelper_unref (gsv.helper);
  g_variant_unref (source);

  check (child);

  return child;
}

/**
 * g_variant_n_children:
 * @container: a container #GVariant
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
 * only on the signature).  For dictionary entries, it is always 2.
 *
 * This function never fails.
 * TS
 **/
gsize
g_variant_n_children (GVariant *container)
{
  GSVariant gsv = { container->signature };
  GVariant *source;
  gsize n_children;

  check (container);

  if (container->representation == G_VARIANT_TREE)
    {
      gsize n_children;

      /* if the ->representation is TREE we have to assume that it
       * might change at any time that we are unlocked.  be careful.
       */
      g_variant_lock (container);

      if (G_UNLIKELY (container->representation != G_VARIANT_TREE))
      {
        /* it changed representation */

        g_variant_unlock (container);
        goto not_a_tree;
      }

      n_children = container->contents.tree.n_children;

      g_variant_unlock (container);

      return n_children;
    }

 not_a_tree:
  gsv = g_variant_get_gsv (container, &source);
  n_children = g_svariant_n_children (gsv);
  g_variant_unref (source);

  return n_children;
}

/**
 * g_variant_get_signature:
 * @value: a #GVariant
 * @returns: a #GSignature
 *
 * Determines the signature of @value.  This is not the function to
 * access the contents of a signature #GVariant instance -- use
 * g_variant_get_string() for that.
 *
 * The return value is valid for the lifetime of @value and must not
 * be freed.
 */
GSignature
g_variant_get_signature (GVariant *value)
{
  check (value);
  return g_svhelper_signature (value->signature);
}

/* private */
gboolean
g_variant_has_signature (GVariant  *value,
                         GSVHelper *signature)
{
  check (value);
  return g_svhelper_equal (value->signature, signature);
}

#if 0
/*
 * g_variant_copy_self:
 * @value: a #GVariant
 *
 * Takes a large serialised value that uses its parent's data and
 * makes a copy of the data for itself.  This allows the reference on
 * the parent to be released.
 *
 * This function is -not- threadsafe with respect to @value.  It must
 * only be called on values for which only one reference exists or
 * while holding the byte-swapping mutex.
 */
static void
g_variant_copy_self (GVariant *value)
{
  gboolean native;
  guchar *new;

  g_assert (value->representation == G_VARIANT_LARGE);
  g_assert (value->contents.large.source != NULL);

  new = g_slice_alloc (value->contents.large.size);
  native = g_variant_copy_safely (value->contents.large.source,
                                  new, value->contents.large.data,
                                  value->contents.large.size);
  value->native_endian = native;

  g_variant_lock (value);
  value->contents.large.data = new;
  g_variant_unref (value->contents.large.source);
  value->contents.large.source = NULL;
  g_variant_unlock (value);
}
#endif

/*
 * g_variant_ensure_native_endian:
 * @value: a #GVariant
 *
 * Ensures that the ->data of @value is stored in machine native byte
 * order.
 *
 * This call can possibly change the ->data pointer (by calling
 * g_variant_copy_self()) and therefore must be called before ->data
 * is ever made visible to the user.  After the value is in machine
 * native byte order, ->data will never change again.
 */
void
g_variant_ensure_native_endian (GVariant *value)
{
  check (value);

  /* bypass grabbing the lock in the common case */
  if (value->native_endian)
    return;

  /* if it was small, it would already be native */
  g_assert (value->representation == G_VARIANT_LARGE);

  g_variant_lock (value);

  if (!value->native_endian && value->contents.large.source)
    {
      GVariant *source = value->contents.large.source;
      gboolean native;
      guchar *new;

      /* if our source became native-endian under us this can
       * save some work.  this is not required for thread-safety
       * as g_variant_copy_safely() will ensure we do the right
       * thing in any case.  this merely saves extra work.
       */
      value->native_endian = source->native_endian;

      if (!value->native_endian)
        {
          new = g_slice_alloc (value->contents.large.size);
          native = g_variant_copy_safely (source, new,
                                          value->contents.large.data,
                                          value->contents.large.size);

          /* just incase source change endian since we checked */
          value->native_endian = native;

          value->contents.large.data = new;
          g_variant_unref (value->contents.large.source);
          value->contents.large.source = NULL;
        }
    }

  g_assert (value->contents.large.source == NULL);

  if (!value->native_endian)
    {
      GSVariant gsv = { value->signature,
                        value->contents.large.data,
                        value->contents.large.size };

      g_svariant_byteswap (gsv);

      value->native_endian = TRUE;
    }

  g_variant_unlock (value);

  check (value);
}

/**
 * g_variant_load:
 * @signature: the #GSignature of the new variant
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
GVariant *
g_variant_load (GSignature    signature,
                gconstpointer data,
                gsize         size,
                GVariantFlags flags)
{
  GSVHelper *helper;
  GVariant *value;
  gpointer mydata;

  if (signature == NULL)
    {
      GVariant *variant;

      variant = g_variant_load (G_SIGNATURE_VARIANT, data, size, flags);
      value = g_variant_get_child (variant, 0);
      g_variant_unref (variant);

      return value;
    }

  helper = g_svhelper_get (signature);

  if (size <= G_VARIANT_SMALL_SIZE)
    {
      value = g_variant_alloc (G_VARIANT_SMALL, helper);
      mydata = value->contents.small.data;
      value->contents.small.size = size;
    }
  else
    {
      mydata = g_slice_alloc (size);

      value = g_variant_alloc (G_VARIANT_LARGE, helper);
      value->contents.large.source = NULL;
      value->contents.large.data = mydata;
      value->contents.large.size = size;
    }

  memcpy (mydata, data, size);

  value->trusted = TRUE;

  return g_variant_apply_flags (value, flags);
}

/*
 * g_variant_gsv_filler:
 * @svalue: the #GSVariant to fill
 * @data: our #GVariant instance
 *
 * Utility function used as a callback from the serialiser to get
 * information about a given #GVariant instance (in @data).
 */
static void
g_variant_gsv_filler (GSVariant *svalue,
                      gpointer   data)
{
  GVariant *value = data;

  check (value);

  if (svalue->helper == NULL)
    svalue->helper = value->signature;

  if (svalue->size == 0)
    svalue->size = g_variant_get_size (value);

  g_assert (svalue->helper == value->signature);
  g_assert (svalue->size == g_variant_get_size (value));

  /* in theory, svalue->data should be NULL if svalue->size is zero
   * but the way the serialiser code is written, it often doesn't
   * know the size of what it is serialising -- only where to put it.
   * by not asserting this case, we allow this function to be called
   * once instead of twice.  in the case of zero size, it simply does
   * nothing.
   */

  if (svalue->data && svalue->size)
    g_variant_store (value, svalue->data);
}

/**
 * g_variant_store:
 * @value: the #GVariant to store
 * @data: the location to store the serialised data at
 *
 * Stores the serialised form of @variant at @data.  @data should be
 * large enough.  See g_variant_get_size().
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

 check_again:
  switch (value->representation)
  {
    case G_VARIANT_TREE:
      {
        GSVariant container;

        container.helper = value->signature;
        container.data = data;
        container.size = g_variant_get_size (value);

        /* if the ->representation is TREE we have to assume that it
         * might change at any time that we are unlocked.  be careful.
         */
        g_variant_lock (value);

        if (G_UNLIKELY (value->representation != G_VARIANT_TREE))
          {
            /* it changed representation */
            g_variant_unlock (value);
            goto check_again;
          }

        g_svariant_serialise (container, &g_variant_gsv_filler,
                              (gpointer *) value->contents.tree.children,
                              value->contents.tree.n_children);

        g_variant_unlock (value);
        check (value);

        break;
      }

    case G_VARIANT_SMALL:
      memcpy (data, value->contents.small.data,
              value->contents.small.size);
      break;

    case G_VARIANT_LARGE:
      g_variant_ensure_native_endian (value);

      memcpy (data, value->contents.large.data,
              value->contents.large.size);
      break;

    default:
      g_assert_not_reached ();
  }
}

/**
 * g_variant_get_data:
 * @value: a #GVariant instance
 * @returns: the serialised form of @value
 *
 * Returns a pointer to the serialised form of a #GVariant instance.
 * The returned data is in machine native byte order but may not be in
 * fully-normalised form if read from an untrusted source.  The
 * returned data must not be freed; it remains valid for as long as
 * @value exists.
 *
 * In the case that @value is already in serialised form, this
 * function is O(1).  If the value is not already in serialised form,
 * serialisation occurs implicitly and is approximately O(n) in the
 * size of the result.
 *
 * This function never fails.
 **/
gconstpointer
g_variant_get_data (GVariant *value)
{
  check (value);

 check_again:
  switch (value->representation)
  {
    case G_VARIANT_TREE:
      {
        GSVariant container;
        GVariant **children;
        gsize n_children;
        gsize i;

        container.helper = value->signature;
        container.size = g_variant_get_size (value);

        /* if the ->representation is TREE we have to assume that it
         * might change at any time that we are unlocked.  be careful.
         */
        g_variant_lock (value);

        if (G_UNLIKELY (value->representation != G_VARIANT_TREE))
          {
            /* it changed representation */
            g_variant_unlock (value);
            goto check_again;
          }

        children = value->contents.tree.children;
        n_children = value->contents.tree.n_children;

        if (container.size <= G_VARIANT_SMALL_SIZE)
          {
            value->representation = G_VARIANT_SMALL;
            container.data = value->contents.small.data;
            value->contents.small.size = container.size;
          }
        else
          {
            value->representation = G_VARIANT_LARGE;
            container.data = g_slice_alloc (container.size);
            value->contents.large.source = NULL;
            value->contents.large.data = container.data;
            value->contents.large.size = container.size;
          }

        g_svariant_serialise (container, &g_variant_gsv_filler,
                              (gpointer *) children, n_children);

        /* always */
        value->native_endian = TRUE;

        for (i = 0; i < n_children; i++)
          g_variant_unref (children[i]);

        g_slice_free1 (sizeof (GVariant *) * n_children, children);

        g_variant_unlock (value);
        check (value);

        return container.data;
      }

    case G_VARIANT_SMALL:
      return value->contents.small.data;

    case G_VARIANT_LARGE:
      g_variant_ensure_native_endian (value);

      return value->contents.large.data;

    default:
      g_assert_not_reached ();
  }
}

/**
 * g_variant_get_size:
 * @value: a #GVariant instance
 * @returns: the serialised size of @value
 *
 * Determines the number of bytes that would be required to store
 * @value with g_variant_store().
 *
 * In the case that @value is already in serialised form or the size
 * has already been calculated (ie: this function has been called
 * before) then this function is O(1).  Otherwise, the size is
 * calculated, an operation which is approximately O(n) in the number
 * of values involved.
 *
 * This function never fails.
 **/
gsize
g_variant_get_size (GVariant *value)
{
  int size;

  check (value);

 check_again:
  switch (value->representation)
  {
    case G_VARIANT_TREE:
      /* if the ->representation is TREE we have to assume that it
       * might change at any time that we are unlocked.  be careful.
       */
      g_variant_lock (value);

      if (G_UNLIKELY (value->representation != G_VARIANT_TREE))
        {
          /* it changed representation */
          g_variant_unlock (value);
          goto check_again;
        }

      /* we're still holding the lock here, so we can be sure that
       * only one thread will go off and calculate the size
       */
      if (value->contents.tree.serialised_size == -1)
        value->contents.tree.serialised_size =
          g_svariant_needed_size (value->signature, &g_variant_gsv_filler,
                                  (gpointer *) value->contents.tree.children,
                                  value->contents.tree.n_children);

      size = value->contents.tree.serialised_size;

      g_variant_unlock (value);
      check (value);

      return size;

    case G_VARIANT_SMALL:
      return value->contents.small.size;

    case G_VARIANT_LARGE:
      return value->contents.large.size;

    default:
      g_assert_not_reached ();
  }
}

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
  GSVariant gsv;

  g_assert (value != NULL);

  g_variant_lock (value);

  g_assert_cmpint (value->ref_count, >, 0);
  g_assert (value->signature != NULL);

  switch (value->representation)
  {
    case G_VARIANT_TREE:
      g_variant_unlock (value);
      return;

    case G_VARIANT_SMALL:
      g_assert_cmpint (value->contents.small.size, <=, G_VARIANT_SMALL_SIZE);
      g_assert (value->native_endian);

      gsv.helper = value->signature;
      gsv.size = value->contents.small.size;
      gsv.data = value->contents.small.data;
      break;

    case G_VARIANT_LARGE:
      g_assert_cmpint (value->contents.large.size, >, G_VARIANT_SMALL_SIZE);

      if (value->contents.large.source)
        {
          GVariant *source = value->contents.large.source;

          g_assert (source->native_endian || !value->native_endian);
          g_assert (source->representation == G_VARIANT_LARGE ||
                    source->representation == G_VARIANT_NOTIFY);

          if (source->representation == G_VARIANT_LARGE)
            {
              /* no grandparents */
              g_assert (source->contents.large.source == NULL);
              g_variant_assert_invariant (source);
            }
        }

      gsv.helper = value->signature;
      gsv.size = value->contents.large.size;
      gsv.data = value->contents.large.data;
      break;

    default:
      g_assert_not_reached ();
  }

  g_svariant_assert_invariant (gsv);
  g_variant_unlock (value);
}

/* private
 *
 * g_variant_is_normalised:
 * @value: a #GVariant instance
 * @returns: %TRUE if @value is definitely in normal form
 *
 * Determines if @value is definitely normalised.  If a value is
 * normalised then the output of any store operations is guaranteed to
 * be in normal form, as is the data obtained from
 * g_variant_get_data().
 *
 * The return value of this function may suddenly change from %FALSE
 * to %TRUE at any time.  The reverse will never occur in the lifetime
 * of @value.
 *
 * It is possible (even likely) that the output might still be in
 * normal form even if this function returns %FALSE.  This function
 * returning %TRUE merely indicates that it will definitely be in
 * normal form.
 *
 * It is an error to use the %G_VARIANT_TRUSTED flag when loading
 * values that are not in normal form.  If you have done this then
 * this function might return invalid results.
 *
 * You should think twice about using this function in most
 * situations.  If you want to have a value that is definitely
 * normalised, use g_variant_normalise().  Don't bother calling this
 * function before conditionally calling g_variant_normalise() -- the
 * check is already done internally.
 */
gboolean
g_variant_is_normalised (GVariant *value)
{
  return value->trusted;
}

/**
 * g_variant_normalise:
 * @value: a possibly non-normalised #GVariant instance
 * @returns: a definitely normalised #GVariant instance
 *
 * Normalises a #GVariant instance.
 *
 * This call consumes the user's reference on @value and returns a
 * reference to the result.  The return value may or may not be the
 * same as the value passed to the function -- this is left entirely
 * unspecified.  Even if the same value is returned, it is important
 * to understand that the user's reference to @value is lost for the
 * duration of the call.  Specifically, if the user only has a single
 * reference then it is not permitted to use this reference from
 * another thread while this call is in progress.
 **/
GVariant *
g_variant_normalise (GVariant *value)
{
  GVariant *source;
  GSVariant gsv;

  if (value->trusted)
    return value;

  gsv = g_variant_get_gsv (value, &source);

  /* we can proceed unlocked.  even if source is in the middle of
   * being byteswapped, it doesn't matter since the byteswap operation
   * preserve normalisation at all times (since normalisation depends
   * only on framing information and byteswapping only affects data).
   *
   * even in the rare case where we are copying ourselves while doing
   * this, we will know the answer for the normality of the copy based
   * on the normality of the source.
   */

  if (g_svariant_is_normalised (gsv))
    {
      value->trusted = TRUE;
      return value;
    }

  /* OK.  we're not normalised.  not good.
   * this case can be as slow as it wants, since it "never happens"...
   */
  g_error ("sorry.  can't normalise non-normal values yet..."); /* XXX */
}
