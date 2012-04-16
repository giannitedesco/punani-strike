/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/asset.h>
#include <punani/blob.h>
#include <math.h>

#include "assetfile.h"

asset_file_t asset_file_open(const char *fn)
{
	struct _asset_file *f = NULL;
#if !ASSET_USE_FLOAT
	unsigned int i;
#endif
	const fp_t *norms;

	f = calloc(1, sizeof(*f));
	if ( NULL == f )
		goto out;

	f->f_buf = blob_from_file(fn, &f->f_sz);
	if ( NULL == f->f_buf )
		goto out_free;

	f->f_hdr = (struct assetfile_hdr *)f->f_buf;
	f->f_desc = (struct asset_desc *)(f->f_buf + sizeof(*f->f_hdr));

	if ( f->f_hdr->h_magic != ASSETFILE_MAGIC ) {
		printf("asset: %s: bad magic\n", fn);
		goto out_free_blob;
	}

	f->f_verts = (fp_t *)(f->f_buf + sizeof(*f->f_hdr) +
			sizeof(*f->f_desc) * f->f_hdr->h_num_assets);
	norms = f->f_verts + 3 * f->f_hdr->h_verts;
	f->f_idx_begin = (idx_t *)(norms + 3 * f->f_hdr->h_verts);

	f->f_db = calloc(f->f_hdr->h_num_assets, sizeof(*f->f_db));
	if ( NULL == f->f_db )
		goto out_free_blob;

#if ASSET_USE_FLOAT
	f->f_norms = norms;
#else
	f->f_norms = calloc(f->f_hdr->h_verts, 3 * sizeof(*f->f_norms));
	if ( NULL == f->f_norms )
		goto out_free_db;

	for(i = 0; i < f->f_hdr->h_verts; i++)
		((float *)f->f_norms)[i] = fp_to_float(norms[i]);
#endif

	/* success */
	f->f_ref = 1;
	goto out;

#if !ASSET_USE_FLOAT
out_free_db:
	free(f->f_db);
#endif
out_free_blob:
	blob_free((void *)f->f_buf, f->f_sz);
out_free:
	free(f);
	f = NULL;
out:
	return f;
}

static asset_file_t ref(asset_file_t f)
{
	f->f_ref++;
	return f;
}

static void unref(asset_file_t f)
{
	if ( f ) {
		f->f_ref--;
		if ( !f->f_ref) {
			blob_free((void *)f->f_buf, f->f_sz);
#if !ASSET_USE_FLOAT
			free((void *)f->f_norms);
#endif
			free(f->f_db);
			free(f);
		}
	}
}

void asset_file_close(asset_file_t f)
{
	unref(f);
}

static const struct asset_desc *find_asset(asset_file_t f, const char *name)
{
	const struct asset_desc *ptr;
	unsigned int i, n;

	ptr = f->f_desc;
	n = f->f_hdr->h_num_assets;

	while(n) {
		int cmp;
		i = n / 2;
		cmp = strncmp(name, (char *)ptr[i].a_name, ASSET_NAMELEN);
		if ( cmp < 0 ) {
			n = i;
		}else if ( cmp > 0 ) {
			ptr = ptr + (i + 1);
			n = n - (i + 1);
		}else{
			return ptr + i;
		}
	}

	return NULL;
}

asset_t asset_file_get(asset_file_t f, const char *name)
{
	const struct asset_desc *d;
	struct _asset *a = NULL;
	unsigned int idx;

	d = find_asset(f, name);
	if ( NULL == d) {
		printf("asset: lookup failed: %s\n", name);
		goto out;
	}

	idx = d - f->f_desc;
	assert(idx < f->f_hdr->h_num_assets);

	/* cache hit */
	if ( f->f_db[idx] ) {
		f->f_db[idx]->a_ref++;
		ref(f);
		return f->f_db[idx];
	}

	a = calloc(1, sizeof(*a));
	if ( NULL == a )
		goto out;

	a->a_indices = f->f_idx_begin + d->a_off;

	/* success */
	f->f_db[idx] = a;
	a->a_idx = idx;
	a->a_owner = ref(f);
	a->a_ref = 1;
	goto out;

out:
	return a;
}

void asset_put(asset_t a)
{
	if ( a ) {
		a->a_ref--;
		if ( !a->a_ref ) {
			a->a_owner->f_db[a->a_idx] = NULL;
			unref(a->a_owner);
			free(a);
		}
	}
}
