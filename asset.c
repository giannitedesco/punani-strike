/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/vec.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/asset.h>
#include <punani/blob.h>
#include <punani/punani_gl.h>
#include <math.h>

#include "list.h"
#include "assetfile.h"

static LIST_HEAD(assets);

static unsigned int num_indices(struct _asset_file *f)
{
	unsigned int i, ret = 0;
	for(i = 0; i < f->f_hdr->h_num_assets; i++) {
		const struct asset_desc *d = f->f_desc + i;
		assert(!(d->a_num_idx % 3));
		ret += d->a_num_idx;
	}
	return ret;
}

static struct _asset_file *do_open(const char *fn)
{
	struct _asset_file *f = NULL;
	const uint8_t *end;

	f = calloc(1, sizeof(*f));
	if ( NULL == f )
		goto out;

	f->f_buf = blob_from_file(fn, &f->f_sz);
	if ( NULL == f->f_buf )
		goto out_free;

	end = f->f_buf + f->f_sz;

	f->f_hdr = (struct assetfile_hdr *)f->f_buf;
	f->f_desc = (struct asset_desc *)(f->f_buf + sizeof(*f->f_hdr));

	if ( f->f_hdr->h_magic != ASSETFILE_MAGIC ) {
		con_printf("asset: %s: bad magic\n", fn);
		goto out_free_blob;
	}

	f->f_verts = (struct asset_vbo *)(f->f_buf + sizeof(*f->f_hdr) +
			sizeof(*f->f_desc) * f->f_hdr->h_num_assets);
	f->f_idx_begin = (idx_t *)(f->f_verts + f->f_hdr->h_verts);
	if ( (uint8_t *)f->f_idx_begin > end )
		goto out_free_blob;

	f->f_db = calloc(f->f_hdr->h_num_assets, sizeof(*f->f_db));
	if ( NULL == f->f_db )
		goto out_free_blob;

	f->f_name = strdup(fn);
	if ( NULL == f->f_name )
		goto out_free_db;

	f->f_verts_ex = calloc(3 * sizeof(*f->f_verts_ex),
				2 * f->f_hdr->h_verts);
	if ( NULL == f->f_verts_ex )
		goto out_free_name;

	f->f_num_indices = num_indices(f);
	f->f_shadow_indices = /* two caps for each asset */
				f->f_hdr->h_num_assets * 6 +
				/* plus 6 triangles for each triangle */
				f->f_num_indices * 6;
	f->f_idx_shadow = calloc(sizeof(*f->f_verts_ex), f->f_shadow_indices);
	if ( NULL == f->f_idx_shadow )
		goto out_free_verts_ex;

	/* success */
	f->f_ref = 1;
	list_add_tail(&f->f_list, &assets);
	goto out;

out_free_verts_ex:
	free(f->f_verts_ex);
out_free_name:
	free(f->f_name);
out_free_db:
	free(f->f_db);
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

asset_file_t asset_file_open(const char *fn)
{
	struct _asset_file *f;

	list_for_each_entry(f, &assets, f_list) {
		if ( !strcmp(f->f_name, fn) )
			return ref(f);
	}
	return do_open(fn);
}

static void unref(asset_file_t f)
{
	if ( f ) {
		f->f_ref--;
		if ( !f->f_ref) {
			glDeleteBuffers(1, &f->f_vbo_geom);
			glDeleteBuffers(1, &f->f_ibo_geom);
			glDeleteBuffers(1, &f->f_vbo_shadow);
			glDeleteBuffers(1, &f->f_ibo_shadow);
			blob_free((void *)f->f_buf, f->f_sz);
			list_del(&f->f_list);
			free(f->f_idx_shadow);
			free(f->f_verts_ex);
			free(f->f_name);
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
		con_printf("asset: lookup failed: %s\n", name);
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
	if ( (uint8_t *)(a->a_indices + d->a_num_idx) > (f->f_buf + f->f_sz) )
		goto out_free;

	/* success */
	f->f_db[idx] = a;
	a->a_idx = idx;
	a->a_owner = ref(f);
	a->a_ref = 1;
	f->f_shadows_dirty = 1;
	goto out;

out_free:
	free(a);
	a = NULL;
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

void assets_recalc_shadow_vols(light_t l)
{
	struct _asset_file *f;

	list_for_each_entry(f, &assets, f_list) {
		f->f_shadows_dirty = 1;
	}
}

void asset_file_dirty_shadows(asset_file_t f)
{
	f->f_shadows_dirty = 1;
}

float asset_radius(asset_t a)
{
	struct _asset_file *f = a->a_owner;
	const struct asset_desc *d = f->f_desc + a->a_idx;
	return d->a_radius;
}

void asset_mins(asset_t a, vec3_t mins)
{
	struct _asset_file *f = a->a_owner;
	const struct asset_desc *d = f->f_desc + a->a_idx;
	v_copy(mins, d->a_mins);
}

void asset_maxs(asset_t a, vec3_t maxs)
{
	struct _asset_file *f = a->a_owner;
	const struct asset_desc *d = f->f_desc + a->a_idx;
	v_copy(maxs, d->a_maxs);
}

int asset_collide_line(asset_t a, const vec3_t start,
			const vec3_t end, vec3_t hit)
{
	struct _asset_file *f = a->a_owner;
	const struct asset_desc *d = f->f_desc + a->a_idx;

	if ( collide_box_line(d->a_mins, d->a_maxs, start, end, hit) ) {
		//printf("Collide %s!!\n", d->a_name);
		return 1;
	}

	return 0;
}

#define SQUARE(x) ((x) * (x))
int asset_collide_sphere(asset_t a, const vec3_t c, float r)
{
	struct _asset_file *f = a->a_owner;
	const struct asset_desc *d = f->f_desc + a->a_idx;
	unsigned int i;
	float dmin = 0.0, r2 = SQUARE(r);

	for(i = 0; i < 3; i++) {
		if( c[i] < d->a_mins[i] )
			dmin += SQUARE(c[i] - d->a_mins[i]);
		else if( c[i] > d->a_maxs[i] )
			dmin += SQUARE(c[i] - d->a_maxs[i]);
	}

	return (dmin <= r2);
}

int asset_sweep(asset_t a, const struct obb *sweep)
{
	struct _asset_file *f = a->a_owner;
	const struct asset_desc *d = f->f_desc + a->a_idx;
	struct obb obb;

	obb_from_aabb(&obb, d->a_mins, d->a_maxs);

	return 1;
	return collide_obb(&obb, sweep);
}

struct tri {
	vec3_t p[3];
};

typedef enum { m3, m21, m12, m111 } ProjectionMap;

struct Config {
	ProjectionMap map;
	int index[3];
	float min, max;
};

static void get_config(struct Config *config, vec3_t D,
			struct tri *U, float speed)
{

	float d0, d1, d2;

	d0 = v_dot_product(D, U->p[0]);
	d1 = v_dot_product(D, U->p[1]);
	d2 = v_dot_product(D, U->p[2]);

	if ( d0 <= d1 ) {
		if ( d1 <= d2 ) {
			config->index[0] = 0;
			config->index[1] = 1;
			config->index[2] = 2;
			config->min = f_min(d0, d0 + speed);
			config->max = f_max(d2, d2 + speed);
			if ( d0 != d1 )
				config->map = (d1 != d2) ? m111 : m12;
			else
				config->map = (d1 != d2) ? m21 : m3;
		}else if ( d0 <= d2 ) {
			config->index[0] = 0;
			config->index[1] = 2;
			config->index[2] = 1;
			config->min = f_min(d0, d0 + speed);
			config->max = f_max(d1, d1 + speed);
			config->map = (d0 != d2) ? m111 : m21;
		}else{
			config->index[0] = 2;
			config->index[1] = 0;
			config->index[2] = 1;
			config->min = f_min(d2, d2 + speed);
			config->max = f_max(d1, d1 + speed);
			config->map = (d0 != d1) ? m12 : m111;
		}
	}else{
		if ( d2 <= d1 ) {
			config->index[0] = 2;
			config->index[1] = 1;
			config->index[2] = 0;
			config->min = f_min(d2, d2 + speed);
			config->max = f_max(d0, d0 + speed);
			config->map = (d2 != d1) ? m111 : m21;
		}else if ( d2 <= d0 ) {
			config->index[0] = 1;
			config->index[1] = 2;
			config->index[2] = 0;
			config->min = f_min(d1, d1 + speed);
			config->max = f_max(d0, d0 + speed);
			config->map = (d2 != d0) ? m111 : m12;
		}else{
			config->index[0] = 1;
			config->index[1] = 0;
			config->index[2] = 2;
			config->min = f_min(d1, d1 + speed);
			config->max = f_max(d2, d2 + speed);
			config->map = m111;
		}
	}
}

#define LEFT -1
#define RIGHT 1

static int Update(struct Config *UC, struct Config *VC, float speed,
		  int *side, struct Config *TUC, struct Config *TVC,
		  vec2_t times)
{
	float T;

//	printf("  -- U: %f %f, V %f %f\n",
//		UC->min, UC->max, VC->min, VC->max);

	if ( VC->max < UC->min ) {
		if ( speed <= 0 )
			return 0;
		T = (UC->min - VC->max) / speed;
		if ( T > times[0] ) {
			times[0] = T;
			*side = LEFT;
			memcpy(TUC, UC, sizeof(*TUC));
			memcpy(TVC, VC, sizeof(*TVC));
		}

		T = (UC->max - VC->min) / speed;
		if ( T < times[1] )
			times[1] = T;
		if ( times[0] > times[1] )
			return 0;
	}else if ( UC->max < VC->min ) {
		if ( speed >= 0 )
			return 0;
		T = (UC->max - VC->min) / speed;
		if ( T > times[0] ) {
			times[0] = T;
			*side = RIGHT;
			memcpy(TUC, UC, sizeof(*TUC));
			memcpy(TVC, VC, sizeof(*TVC));
		}

		T = (UC->min - VC->max) / speed;
		if ( T < times[1] )
			times[1] = T;
		if ( times[0] > times[1] )
			return 0;
	}else{
		if ( speed > 0 ) {
			T = (UC->max - VC->min) / speed;
			if ( T < times[1] )
				times[1] = T;
			if ( times[0] > times[1] )
				return 0;
		}else if ( speed < 0 ) {
			T = (UC->min - VC->max) / speed;
			if ( T < times[1] )
				times[1] = T;
			if ( times[0] > times[1] )
				return 0;
		}
	}
	return 1;
}

static void tri_normal(vec3_t vec, const struct tri *tri)
{
	vec3_t e1, e2;

	v_sub(e1, tri->p[1], tri->p[0]);
	v_sub(e2, tri->p[2], tri->p[1]);
	v_cross_product(vec, e1, e2);
	v_normalize(vec);
}

static void edge_product(vec3_t vec, const struct tri *a, const struct tri *b,
			unsigned int ai, unsigned int bi)
{
	unsigned int ai2, bi2;
	vec3_t e1, e2;

	ai2 = (ai < 2) ? (ai + 1) : 0;
	bi2 = (bi < 2) ? (bi + 1) : 0;

	v_sub(e1, a->p[ai], a->p[ai2]);
	v_sub(e2, b->p[bi], b->p[bi2]);
	v_cross_product(vec, e1, e2);
	v_normalize(vec);
}

static int intersect_tris(struct tri *U, struct tri *V,
				const vec3_t W,
				struct Config *TUC,
				struct Config *TVC,
				vec2_t times)
{
	unsigned int i, j;
	struct Config UC, VC;
	vec3_t D;
	float speed;
	int side = 0;

//	printf("Triangle test\n");

	/* face normal of U */
	tri_normal(D, U);
	speed = v_dot_product(D, W);
	get_config(&UC, D, U, 0);
	get_config(&VC, D, V, speed);
//	printf(" - Normal U (speed %f)\n", speed);
	if ( !Update(&UC, &VC, speed, &side, TUC, TVC, times) )
		return 0;

	/* face normal of V */
//	printf(" - Normal V (speed %f)\n", speed);
	tri_normal(D, V);
	speed = v_dot_product(D, W);
	get_config(&UC, D, U, 0);
	get_config(&VC, D, V, speed);
	if ( !Update(&UC, &VC, speed, &side, TUC, TVC, times) )
		return 0;

	/* edgewise cross products */
	for(i = 0; i < 3; i++) {
		for(j = 0; j < 3; j++) {
//			printf(" - U%u V%d product\n", i, j);
			edge_product(D, U, V, i, j);
			speed = v_dot_product(D, W);
			get_config(&UC, D, U, 0);
			get_config(&VC, D, V, speed);
			if ( !Update(&UC, &VC, speed, &side, TUC, TVC, times) )
				return 0;
		}
	}
//	printf(" - times %f, %f\n", times[0], times[1]);

	/* hit */
	return 1;
}

static void get_vert(asset_t a, unsigned int idx, vec3_t out)
{
	struct _asset_file *f = a->a_owner;
	const struct asset_vbo *vbo;

	vbo = &f->f_verts[a->a_indices[idx]];
	v_copy(out, vbo->v_vert);
}

int asset_intersect(asset_t a, asset_t b, const mat3_t rot,
			const vec3_t trans, const vec3_t vel)
{
	const struct asset_desc *da = a->a_owner->f_desc + a->a_idx;
	const struct asset_desc *db = b->a_owner->f_desc + b->a_idx;
	struct Config TUC, TVC;
	vec2_t times;
	unsigned int i, j;
	int ret = 0;

//	printf("%d tris x %d tris\n", da->a_num_idx / 3, db->a_num_idx / 3);

	times[0] = 0;
	times[1] = INFINITY;

	for(i = 0; i < da->a_num_idx; i += 3) {
		struct tri tmp, ta;

		get_vert(a, i + 0, tmp.p[0]);
		get_vert(a, i + 1, tmp.p[1]);
		get_vert(a, i + 2, tmp.p[2]);

		/* Rotate verts in A */
		basis_transform(rot, ta.p[0], tmp.p[0]);
		basis_transform(rot, ta.p[1], tmp.p[1]);
		basis_transform(rot, ta.p[2], tmp.p[2]);

		/* translate verts in A */
		v_sub(ta.p[0], ta.p[0], trans);
		v_sub(ta.p[1], ta.p[1], trans);
		v_sub(ta.p[2], ta.p[2], trans);

		for(j = 0; j < db->a_num_idx; j += 3) {
			struct tri tb;

			get_vert(b, j + 0, tb.p[0]);
			get_vert(b, j + 1, tb.p[1]);
			get_vert(b, j + 2, tb.p[2]);

			/* everything in final world space now */
			if ( intersect_tris(&ta, &tb, vel, &TUC, &TVC, times) )
				ret = 1;
		}
	}

	if ( ret && times[0] < times[1] && times[0] <= 1.0)
		printf("Hit %f %f\n", times[0], times[1]);
	return (ret && times[0] < times[1] && times[0] <= 1.0);
}

void asset_render2(asset_t a, renderer_t r,
			const mat3_t rot, const vec3_t trans)
{
	const struct asset_desc *da = a->a_owner->f_desc + a->a_idx;
	unsigned int i;

	renderer_wireframe(r, 1);
	glEnable(GL_DEPTH_TEST);
	glBegin(GL_TRIANGLES);
	for(i = 0; i < da->a_num_idx; i += 3) {
		struct tri tmp, ta;

		get_vert(a, i + 0, tmp.p[0]);
		get_vert(a, i + 1, tmp.p[1]);
		get_vert(a, i + 2, tmp.p[2]);

		/* Rotate verts in A */
		basis_transform(rot, ta.p[0], tmp.p[0]);
		basis_transform(rot, ta.p[1], tmp.p[1]);
		basis_transform(rot, ta.p[2], tmp.p[2]);

		/* translate verts in A */
		v_add(ta.p[0], ta.p[0], trans);
		v_add(ta.p[1], ta.p[1], trans);
		v_add(ta.p[2], ta.p[2], trans);

		glVertex3fv(ta.p[0]);
		glVertex3fv(ta.p[1]);
		glVertex3fv(ta.p[2]);

	}
	glEnd();
	renderer_wireframe(r, 0);
}
