/*
* This file is part of Firestorm NIDS
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "hgang.h"

#if MPOOL_POISON
#define POISON(ptr, len) memset(ptr, MPOOL_POISON_PATTERN, len)
#else
#define POISON(ptr, len) do { } while(0);
#endif

/** hgang descriptor.
 * \ingroup g_hgang
*/
struct _hgang {
	/** Object size. */
	size_t obj_size;
	/** Pointer to next available object */
	uint8_t *next_obj;
	/** Size of each block including hgang_hdr overhead. */
	size_t slab_size;
	/** List of blocks. */
	struct _hgang_hdr *slabs;
	/** List of free'd objects. */
	void *free;
};


size_t hgang_object_size(hgang_t h)
{
	return h->obj_size;
}

/** hgang memory area descriptor.
 * \ingroup g_hgang
*/
struct _hgang_hdr {
	/** Pointer to next item in the list */
	struct _hgang_hdr *next;
	/** Data up to the slab_size */
	uint8_t data[0];
};

static uint8_t *first_byte(struct _hgang_hdr *hdr)
{
	return (uint8_t *)hdr + sizeof(*hdr);
}

static uint8_t *last_byte(struct _hgang *h, struct _hgang_hdr *hdr)
{
	return (uint8_t *)hdr + h->slab_size;
}

static int obj_in_slab(struct _hgang *h, struct _hgang_hdr *hdr, void *obj)
{
	return ((uint8_t *)obj >= first_byte(hdr) &&
		(uint8_t *)obj + h->obj_size <= last_byte(h, hdr));
}

/** Initialise an hgang.
 * \ingroup g_hgang
 *
 * @param h an hgang structure to use
 * @param obj_size size of objects to allocate
 * @param slab_size size of slabs in number of objects (set to zero for auto)
 *
 * Creates a new empty memory pool descriptor with the passed values
 * set. The resultant hgang has no alignment requirement set.
 *
 * @return zero on error, non-zero for success
 * (may only return 0 if the obj_size is 0).
 */
hgang_t hgang_new(size_t obj_size, unsigned slab_size)
{
	struct _hgang *h;

	/* quick sanity checks */
	if ( obj_size == 0 )
		return NULL;

	h = malloc(sizeof(*h));
	if ( NULL == h )
		return NULL;

	if ( obj_size < sizeof(void *) )
		obj_size = sizeof(void *);

	h->obj_size = obj_size;

	if ( slab_size ) {
		h->slab_size = sizeof(struct _hgang_hdr) + 
				(slab_size * obj_size);
	}else{
		/* XXX: totally arbitrary... */
		if ( h->obj_size < 8192 ) {
			h->slab_size = sizeof(struct _hgang_hdr) +
				(8192 - (8192 % h->obj_size));
		}else{
			h->slab_size = sizeof(struct _hgang_hdr) +
				(4 * h->obj_size);
		}
	}

	h->slabs = NULL;
	h->free = NULL;

	return h;
}

/** Slow path for hgang allocations.
 * \ingroup g_hgang
 * @param h a valid hgang structure returned from hgang_init()
 *
 * Allocate a new object, returns NULL if out of memory. Note that this
 * is the slow path, the fast path for common case (no new allocation
 * needed) is handled in the inline fucntion hgang_alloc() in hgang.hdr.
 *
 * @return a new object
 */
static void *hgang_alloc_slow(struct _hgang *h)
{
	struct _hgang_hdr *hdr;
	void *ptr, *ret;

	hdr = ptr = malloc(h->slab_size);
	if ( hdr == NULL )
		return NULL;

	POISON(ptr, h->slab_size);

	/* Set first object */
	ret = ptr + sizeof(*hdr);
	h->next_obj = ret + h->obj_size;

	/* prepend to slab list */
	hdr->next = h->slabs;
	h->slabs = hdr;

	return ret;
}

/** Allocate an object from an hgang.
 * \ingroup g_hgang
 * @param h a valid hgang structure returned from hgang_init()
 *
 * Allocate a new object, returns NULL if out of memory. This is the
 * fast path. It never calls malloc directly.
 *
 * @return a new object
 */
void *hgang_alloc(hgang_t h)
{
	/* Try a free'd object first */
	if ( h->free ) {
		void *ret = h->free;
		h->free = *(void **)h->free;
		return ret;
	}

	/* If there is space in the slab, allocate */
	if ( h->slabs != NULL ) {
		void *ret = h->next_obj;

		if ( obj_in_slab(h, h->slabs, ret) ) {
			h->next_obj += h->obj_size;
			return ret;
		}
	}

	/* Otherwise go to slow path (calls malloc) */
	return hgang_alloc_slow(h);
}

/** Destroy an hgang object.
 * \ingroup g_hgang
 * @param h a valid hgang structure returned from hgang_init()
 *
 * Frees up all allocated memory from the #hgang and resets all members
 * to invalid values.
 */
void hgang_free(hgang_t h)
{
	struct _hgang_hdr *hdr, *f;

	if ( NULL == h )
		return;

	for(hdr = h->slabs; (f = hdr); free(f)) {
		hdr = hdr->next;
		POISON(f, h->slab_size);
	}

	POISON(h, sizeof(*h));
	free(h);
}

/** Free an individual object.
 * \ingroup g_hgang
 * @param h hgang object that obj was allocated from.
 * @param obj pointer to object to free.
 *
 * When an hgang object is free'd it's added to a linked list of
 * free objects which hgang_alloc() scans before trying to commit
 * further memory resources.
*/
void hgang_return(hgang_t h, void *obj)
{
	if ( obj == NULL )
		return;
	assert(h->obj_size >= sizeof(void *));
	POISON(obj, h->obj_size);
	*(void **)obj = h->free;
	h->free = obj;
}

/** Allocate an object initialized to zero.
 * \ingroup g_hgang
 *
 * @param h hgang object to allocate from.
 *
 * This is the same as hgang_alloc() but initializes the returned
 * memory to all zeros.
 *
 * @return pointer to new object or NULL for error
*/
void *hgang_alloc0(hgang_t h)
{
	void *ret;

	ret = hgang_alloc(h);
	if ( ret )
		memset(ret, 0, h->obj_size);

	return ret;
}

/** Call a callback function for each allocated object
 * \ingroup g_hgang
 *
 * @param h hgang object to search
 *
 * WARNING: this code does not check against the free-list so callers will
 * need a way to distinguish free'd objects. Realistically, this function
 * should only be called if hgang_return() has never been c alled on h.
 *
 * @return pointer to new object or NULL for error
*/
int hgang_foreach(hgang_t h, hgang_cb_t cb, void *priv)
{
	struct _hgang_hdr *hdr;
	uint8_t *obj;

	if ( NULL == h->slabs )
		return 1;

	assert(NULL != h->free);

	hdr = h->slabs;
	for(obj = first_byte(hdr);
			(uint8_t *)obj + h->obj_size <= h->next_obj; 
			obj += h->obj_size) {
		if ( !(*cb)(priv, obj) )
			return 0;
	}

	for(hdr = hdr->next; hdr; hdr = hdr->next) {
		for(obj = first_byte(hdr);
				obj_in_slab(h, hdr, obj);
				obj += h->obj_size) {
			if ( !(*cb)(priv, obj) )
				return 0;
		}
	}

	return 1;
}
