/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/asset.h>
#include <punani/blob.h>

#include <punani/assetfile.h>

#include <GL/gl.h>

struct _asset_file {
	const struct assetfile_hdr *f_hdr;
	const struct asset_desc *f_desc;
	struct _asset **f_db;
	const uint8_t *f_buf;
	const fp_t *f_verts;
	const fp_t *f_norms;
	idx_t *f_idx_begin;
	size_t f_sz;
	unsigned int f_ref;
};

struct _asset {
	struct _asset_file *a_owner;
	uint16_t *a_verts;
	uint16_t *a_norms;
	unsigned int a_idx;
	unsigned int a_ref;
};

asset_file_t asset_file_open(const char *fn)
{
	struct _asset_file *f = NULL;

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
		goto out_free;
	}

	f->f_verts = (fp_t *)(f->f_buf + sizeof(*f->f_hdr) +
			sizeof(*f->f_desc) * f->f_hdr->h_num_assets);
	f->f_norms = f->f_verts + 3 * f->f_hdr->h_verts;
	f->f_idx_begin = (idx_t *)(f->f_norms + 3 * f->f_hdr->h_norms);

	f->f_db = calloc(f->f_hdr->h_num_assets, sizeof(*f->f_db));
	if ( NULL == f->f_db )
		goto out_free;

	/* success */
	f->f_ref = 1;
	goto out;
//out_free_blob:
//	blob_free(f->f_buf, f->f_sz);
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
	unsigned int idx, i, v;
	idx_t norm, *arr;

	d = find_asset(f, name);
	if ( NULL == d) {
		printf("asset: lookup failed: %s\n", name);
		goto out;
	}

	idx = d - f->f_desc;
	printf("opening %s: index = %u\n", name, idx);
	assert(idx < f->f_hdr->h_num_assets);

	/* cache hit */
	if ( f->f_db[idx] ) {
		printf(" - %s: cache hit\n", name);
		f->f_db[idx]->a_ref++;
		ref(f);
		return f->f_db[idx];
	}

	a = calloc(1, sizeof(*a));
	if ( NULL == a )
		goto out;

	a->a_verts = calloc(1, 2 * sizeof(*a->a_verts) * d->a_num_verts);
	if ( NULL == a->a_verts ) {
		goto out_free;
	}
	a->a_norms = a->a_verts + d->a_num_verts;

	printf(" - allocated %u verts\n", d->a_num_verts);
	arr = f->f_idx_begin + d->a_off;
	for(norm = i = v = 0; i < d->a_num_cmds; i++) {
		if ( arr[i] & RCMD_NORMAL_FLAG ) {
			norm = arr[i] & ~RCMD_NORMAL_FLAG;
		}else{
			a->a_verts[v] = arr[i];
			a->a_norms[v] = norm;
			assert(a->a_verts[v] < f->f_hdr->h_verts);
			assert(a->a_norms[v] < f->f_hdr->h_norms);
			v++;
		}
	}

	/* success */
	f->f_db[idx] = a;
	a->a_idx = idx;
	a->a_owner = ref(f);
	goto out;

out_free:
	free(a);
out:
	return a;
}

void asset_file_render_begin(asset_file_t f)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_SHORT, 0, f->f_verts);
	glNormalPointer(GL_SHORT, 0, f->f_norms);
}

void asset_file_render_end(asset_file_t f)
{
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void asset_render(asset_t a)
{
	struct asset_desc *d = a->a_owner->f_desc + a->a_idx;
	glDrawElements(GL_TRIANGLES, d->a_num_verts,
			GL_UNSIGNED_SHORT, a->a_verts);
}

void asset_put(asset_t a)
{
	if ( a ) {
		a->a_ref--;
		if ( !a->a_ref ) {
			a->a_owner->f_db[a->a_idx] = NULL;
			unref(a->a_owner);
			free(a->a_verts);
			free(a);
		}
	}
}
