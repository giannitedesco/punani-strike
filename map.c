/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/vec.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/map.h>
#include <punani/asset.h>
#include <punani/tile.h>
#include <punani/blob.h>
#include <punani/punani_gl.h>

#include "dessert-stroke.h"
#include "mapfile.h"

struct _map {
	asset_file_t m_assets;
	midx_t *m_indices;
	tile_t *m_tiles;
	uint8_t *m_buf;
	size_t m_sz;
	int *m_colliding;
	unsigned int m_num_tiles;
	unsigned int m_width;
	unsigned int m_height;
};

struct map_frustum {
	vec3_t q[4];
	float mins[2];
	float maxs[2];
};

static int point_inside(struct map_frustum *f, vec3_t p)
{
	vec3_t c, a, b;
	unsigned int i;

	for(i = 0; i < 4; i++) {
		v_sub(a, f->q[i], f->q[(i + 1) % 4]);
		v_sub(b, p, f->q[(i + 1) % 4]);

		v_cross_product(c, a, b);
		if ( c[Y] > 0 )
			return 0;
	}

	return 1;
}

static int corners_inside(struct map_frustum *f, vec3_t *p)
{
	unsigned int i;

	for(i = 0; i < 4; i++) {
		if ( point_inside(f, p[i]) )
			return 1;
	}

	return 0;
}

static int line_intersects(float x1, float y1, float x2, float y2,
				float x3, float y3, float x4, float y4)
{
	float d;

	d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	if ( d == 0.0 )
		return 0;

	return 1;
}

static int lines_intersect(struct map_frustum *f, vec3_t *p)
{
	unsigned int i, j;
	float x1, x2, x3, x4;
	float y1, y2, y3, y4;

	for(i = 0; i < 4; i++) {
		for(j = 0; j < 4; i++) {
			x1 = f->q[i][X];
			y1 = f->q[i][Z];
			x2 = f->q[(i + 1) % 4][X];
			y2 = f->q[(i + 1) % 4][Z];
			x3 = p[i][X];
			y3 = p[i][Z];
			x4 = p[(i + 1) % 4][X];
			y4 = p[(i + 1) % 4][Z];
			if ( line_intersects(x1, y1, x2, y2, x3, y3, x4, y4) )
				return 1;
		}
	}

	return 0;
}

static int visible(struct map_frustum *f, float x1, float x2,
					float y1, float y2)
{
	vec3_t p[4];

	p[0][X] = x1;
	p[0][Y] = 0.0;
	p[0][Z] = y1;

	p[1][X] = x1;
	p[1][Y] = 0.0;
	p[1][Z] = y2;

	p[2][X] = x2;
	p[2][Y] = 0.0;
	p[2][Z] = y2;

	p[3][X] = x2;
	p[3][Y] = 0.0;
	p[3][Z] = y1;

	if ( !corners_inside(f, p) ) {
		return lines_intersect(f, p);
	}

	return 1;
}

static void render_tile_at(tile_t t, float x, float y,
				renderer_t r, light_t l,
				struct map_frustum *f, int colliding)
{
	if ( !visible(f, x, x + TILE_X, y, y + TILE_Y) ) {
		return;
	}
	glPushMatrix();
	renderer_translate(r, x, 0.0, y);
	tile_render(t, r, l);
	if ( colliding )
		tile_render_bbox(t, r);
	glPopMatrix();
}

static void get_frustum_bbox(renderer_t r, struct map_frustum *f)
{
	unsigned int i;

	renderer_get_frustum_quad(r, 0.0, f->q);

	f->mins[0] = f->mins[1] = 1000.0;
	f->maxs[0] = f->maxs[1] = -1000.0;
	for(i = 0; i < 4; i++) {
		if ( f->q[i][X] < f->mins[X] )
			f->mins[X] = f->q[i][X];
		if ( f->q[i][X] > f->maxs[X] )
			f->maxs[X] = f->q[i][X];
		if ( f->q[i][Z] < f->mins[Y] )
			f->mins[Y] = f->q[i][Z];
		if ( f->q[i][Z] > f->maxs[Y] )
			f->maxs[Y] = f->q[i][Z];
	}
}

static void render_map(map_t m, renderer_t r, light_t l)
{
	struct map_frustum f;
	int i, j;
	int xa, xb, ya, yb;

	get_frustum_bbox(r, &f);

	ya = floor(f.mins[Y] / TILE_Y);
	yb = ceil(f.maxs[Y] / TILE_Y);
	xa = floor(f.mins[X] / TILE_X);
	xb = ceil(f.maxs[X] / TILE_X);

	if ( xa < 0 )
		xa = 0;
	if ( ya < 0 )
		ya = 0;
	if ( xb > (int)m->m_width )
		xb = m->m_width;
	if ( yb > (int)m->m_height )
		yb = m->m_height;

	asset_file_render_begin(m->m_assets, r, l);
	for(i = ya; i < yb; i++) {
		for(j = xa; j < xb; j++) {
			float x, y;
			tile_t t;

			x = (float)j * TILE_X;
			y = (float)i * TILE_Y;

			t = m->m_tiles[m->m_indices[i * m->m_width + j]];
			render_tile_at(t, x, y, r, l, &f,
					m->m_colliding[i * m->m_width + j]);
		}
	}
	asset_file_render_end(m->m_assets);

#if 0
	/* Draw the clip field */
	renderer_wireframe(r, 1);
	glColor4f(1.0, 0.0, 0.0, 1.0);
	glEnable(GL_CULL_FACE);
	glBegin(GL_QUADS);
	for(i = 0; i < 4; i++) {
		glVertex3f(f.q[i][0], 0, f.q[i][2]);
	}
	glEnd();
	renderer_wireframe(r, 0);
#endif
}

int map_collide_line(map_t m, const vec3_t a, const vec3_t b, vec3_t hit)
{
	int mins[2], maxs[2];
	int x, y;
	int ret = 0;

	/* First determine which tiles the line segment crosses */
	mins[0] = floor(f_min(a[0], b[0]) / TILE_X);
	mins[1] = floor(f_min(a[2], b[2]) / TILE_Y);
	maxs[0] = ceil(f_max(a[0], b[0]) / TILE_X);
	maxs[1] = ceil(f_max(a[2], b[2]) / TILE_Y);

	mins[0] = r_max(mins[0], 0);
	mins[1] = r_max(mins[1], 0);
	maxs[0] = r_min(maxs[0], m->m_width);
	maxs[1] = r_min(maxs[1], m->m_height);

#if 0
	printf("%f,%f -> %f,%f :: %u,%u -> %u,%u\n",
		a[0], a[2], b[0], b[2],
		mins[0], mins[1], maxs[0] - 1, maxs[1] - 1);
#endif
	for(y = mins[1]; y < maxs[1]; y++) {
		for(x = mins[0]; x < maxs[0]; x++) {
			vec3_t start, end, h;
			tile_t t;

			/* lookup the tile */
			t = m->m_tiles[m->m_indices[y * m->m_width + x]];

			/* translate the line segment in to tile space */
			v_copy(start, a);
			v_copy(end, b);
			start[0] -= TILE_X * x;
			start[2] -= TILE_Y * y;
			end[0] -= TILE_X * x;
			end[2] -= TILE_Y * y;
#if 0
			printf(" - tile %u,%u: %f,%f,%f %f,%f,%f\n",
				x, y,
				start[0], start[1], start[2],
				end[0], end[1], end[2]);
#endif
			if ( !tile_collide_line(t, start, end, h) )
				continue;

			/* make sure to chose nearest intersection */
			if ( ret ) {
				vec3_t tmp;
				float da, db;

				v_sub(tmp, a, hit);
				da = fabs(v_len(tmp));

				v_sub(tmp, a, h);
				db = fabs(v_len(tmp));

				if ( db < da )
					v_copy(hit, h);
			}else{
				v_copy(hit, h);
				ret = 1;
			}
		}
	}

	return ret;
}

struct shim {
	map_cbfn_t cb;
	map_t m;
	void *priv;
	tile_t tile;
	int x, y;
};

static int tcb(const struct tile_hit *hit, void *priv)
{
	struct shim *shim = priv;
	struct map_hit h;

	h.asset = hit->asset;
	h.tile = shim->tile;
	v_copy(h.origin, hit->origin);
	memcpy(h.times, hit->times, sizeof(h.times));
	h.origin[0] += TILE_X * shim->x;
	h.origin[2] += TILE_Y * shim->y;
	h.map_x = shim->x;
	h.map_y = shim->y;
	h.tile_idx = hit->index;

	shim->m->m_colliding[shim->y * shim->m->m_width + shim->x] = 1;

	return (*shim->cb)(&h, shim->priv);
}

int map_findradius(map_t m, const vec3_t c, float r,
			map_cbfn_t cb, void *priv)
{
	int mins[2], maxs[2];
	struct shim shim;
	int x, y;

	/* First determine which tiles the sphere intersects */
	mins[0] = floor((c[0] - r) / TILE_X);
	mins[1] = floor((c[2] - r) / TILE_Y);
	maxs[0] = ceil((c[0] + r)/ TILE_X);
	maxs[1] = ceil((c[2] + r) / TILE_Y);

	mins[0] = r_max(mins[0], 0);
	mins[1] = r_max(mins[1], 0);
	maxs[0] = r_min(maxs[0], m->m_width);
	maxs[1] = r_min(maxs[1], m->m_height);

	for(y = mins[1]; y < maxs[1]; y++) {
		for(x = mins[0]; x < maxs[0]; x++) {
			vec3_t c2;
			tile_t t;

			/* lookup the tile */
			t = m->m_tiles[m->m_indices[y * m->m_width + x]];

			/* translate the line sphere in to tile space */
			v_copy(c2, c);
			c2[0] -= TILE_X * x;
			c2[2] -= TILE_Y * y;

			shim.cb = cb;
			shim.priv = priv;
			shim.tile = t;
			shim.x = x;
			shim.y = y;
			if ( !tile_collide_sphere(t, c2, r, tcb, &shim) )
				return 0;
		}
	}

	return 1;
}

int map_sweep(map_t m, const struct obb *sweep,
			map_cbfn_t cb, void *priv)
{
	vec3_t smins, smaxs;
	int mins[2], maxs[2];
	struct shim shim;
	vec3_t v;
	int x, y;

	obb_build_aabb(sweep, smins, smaxs);

	//v_sub(v, sweep->b, sweep->a);
	v_zero(v);

	mins[0] = floor( (smins[0] -
			((v[0] < 0) ? v[0] : 0.0)) / TILE_X);
	mins[1] = floor( (smins[2] -
			((v[2] < 0) ? v[2] : 0.0)) / TILE_Y);
	maxs[0] = ceil( (smaxs[0] +
			((v[0] > 0) ? v[0] : 0.0)) / TILE_X);
	maxs[1] = ceil( (smaxs[2] +
			((v[2] > 0) ? v[2] : 0.0)) / TILE_Y);

	mins[0] = r_max(mins[0], 0);
	mins[1] = r_max(mins[1], 0);
	maxs[0] = r_min(maxs[0], m->m_width);
	maxs[1] = r_min(maxs[1], m->m_height);

#if 0
	mins[0] = mins[1] = 0;
	maxs[0] = m->m_width;
	maxs[1] = m->m_height;
#endif

	memset(m->m_colliding, 0, m->m_width * m->m_height * sizeof(*m->m_colliding));

//	printf("tiles: %d,%d -> %d,%d\n",
//		mins[0], mins[1], maxs[0] - 1, maxs[1] - 1);
	for(y = mins[1]; y < maxs[1]; y++) {
		for(x = mins[0]; x < maxs[0]; x++) {
			struct obb obb;
			vec3_t off;
			tile_t t;

			/* lookup the tile */
			t = m->m_tiles[m->m_indices[y * m->m_width + x]];
			off[0] = x * TILE_X;
			off[1] = 0.0;
			off[2] = y * TILE_Y;
			memcpy(&obb, sweep, sizeof(obb));
			v_sub(obb.origin, obb.origin, off);

			shim.m = m;
			shim.cb = cb;
			shim.priv = priv;
			shim.tile = t;
			shim.x = x;
			shim.y = y;
			if ( !tile_sweep(t, &obb, tcb, &shim) )
				return 0;
		}
	}

	return 0;
}

void map_render(map_t m, renderer_t r, light_t l)
{
	render_map(m, r, l);
}

void map_get_size(map_t m, unsigned int *x, unsigned int *y)
{
	if ( x )
		*x = m->m_width;
	if ( y )
		*y = m->m_height;
}

map_t map_load(renderer_t r, const char *name)
{
	const struct map_hdr *hdr;
	struct _map *m = NULL;
	unsigned int i;
	char *names;

	m = calloc(1, sizeof(*m));
	if ( NULL == m )
		goto out;

	m->m_assets = asset_file_open("data/assets.db");
	if ( NULL == m->m_assets )
		goto out_free;

	m->m_buf = blob_from_file(name, &m->m_sz);
	if ( NULL == m->m_buf )
		goto out_free_asset;

	hdr = (struct map_hdr *)m->m_buf;
	if ( m->m_sz < sizeof(*hdr) )
		goto out_free_blob;

	if ( hdr->h_magic != MAPFILE_MAGIC ) {
		fprintf(stderr, "bad magic\n");
		goto out_free_blob;
	}

	m->m_width = hdr->h_x;
	m->m_height = hdr->h_y;
	m->m_num_tiles= hdr->h_num_tiles;

	m->m_tiles = calloc(m->m_num_tiles, sizeof(*m->m_tiles));
	if ( NULL == m->m_tiles )
		goto out_free_blob;

	names = (char *)(m->m_buf + sizeof(*hdr));
	for(i = 0; i < m->m_num_tiles; i++) {
		m->m_tiles[i] = tile_get(m->m_assets,
					names + i * MAPFILE_NAMELEN);
		if ( NULL == m->m_tiles[i] )
			goto out_free_tiles;
	}

	m->m_indices = (midx_t *)(names + m->m_num_tiles * MAPFILE_NAMELEN);
	m->m_colliding = calloc(m->m_width * m->m_height, sizeof(*m->m_colliding));

	/* success */
	goto out;

out_free_tiles:
	free(m->m_tiles);
out_free_blob:
	blob_free(m->m_buf, m->m_sz);
out_free_asset:
	asset_file_close(m->m_assets);
out_free:
	free(m);
	m = NULL;
out:
	return m;
}

void map_free(map_t m)
{
	if ( m ) {
		unsigned int i;
		for(i = 0; i < m->m_num_tiles; i++)
			tile_put(m->m_tiles[i]);
		free(m->m_tiles);
		blob_free(m->m_buf, m->m_sz);
		asset_file_close(m->m_assets);
		free(m);
	}
}
