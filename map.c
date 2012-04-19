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

struct _map {
	asset_file_t m_assets;
	tile_t m_null;
	tile_t m_city;
	tile_t m_mosque;
#if 0
	uint8_t *m_buf;
	size_t m_sz;
#endif
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

static int clip(struct map_frustum *f, float x1, float x2,
					float y1, float y2)
{
	unsigned int i;
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

	for(i = 0; i < 4; i++) {
		if ( point_inside(f, p[i]) )
			return 1;
	}

	return 0;
}

static void render_tile_at(tile_t t, float x, float y,
				renderer_t r, light_t l,
				struct map_frustum *f)
{
	if ( !clip(f, -x, -(x - TILE_X), -y, -(y - TILE_Y)) ) {
		glColor4f(1.0, 0.0, 1.0, 1.0);
		return;
	}
	glPushMatrix();
	renderer_translate(r, -x, 0.0, -y);
	tile_render(t, r, l);
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

#if 0
	/* Draw the clip field */
	renderer_wireframe(r, 1);
	glColor4f(1.0, 0.0, 0.0, 1.0);
	glEnable(GL_CULL_FACE);
	glBegin(GL_QUADS);
	for(i = 0; i < 4; i++) {
		glVertex3f(f->q[i][0], 0, f->q[i][2]);
	}
	glEnd();
	renderer_wireframe(r, 0);
#endif
}

static void render_map(map_t m, renderer_t r, light_t l)
{
	int i, j;
	uint8_t map[10][10] = {
		{2, 0, 0, 1, 1, 0, 0, 0, 0, 1,},
		{0, 1, 0, 1, 1, 1, 0, 0, 2, 0,},
		{0, 0, 1, 0, 1, 0, 2, 0, 1, 0,},
		{1, 1, 1, 1, 1, 1, 0, 1, 0, 1,},
		{0, 1, 0, 1, 1, 0, 0, 1, 1, 0,},
		{0, 1, 1, 0, 2, 0, 1, 1, 0, 0,},
		{0, 1, 1, 0, 1, 0, 0, 0, 1, 1,},
		{0, 1, 1, 2, 1, 0, 1, 0, 0, 0,},
		{0, 0, 0, 0, 0, 0, 0, 0, 1, 0,},
		{0, 0, 2, 0, 0, 0, 0, 0, 0, 1,},
	};
	tile_t tiles[3] = { m->m_null, m->m_city, m->m_mosque };
	struct map_frustum f;

	get_frustum_bbox(r, &f);

	asset_file_render_begin(m->m_assets);
#if 1
	for(i = -1; i < 9; i++) {
		for(j = -1; j < 9; j++) {
			float x, y;
			x = (float)i * TILE_X;
			y = TILE_Y + (float)j * TILE_Y;

			if ( -(x - TILE_X) < f.mins[X] || -x > f.maxs[X] ||
				-(y - TILE_Y) < f.mins[Y] || -y > f.maxs[Y] ) {
				continue;
			}

			tile_t t = tiles[map[i + 1][j + 1]];
			render_tile_at(t, x, y, r, l, &f);
		}
	}
#else
	render_tile_at(m->m_city, -50.0, 25.0, r, l, &f);
	render_tile_at(m->m_null, -50.0, 50.0, r, l, &f);
	render_tile_at(m->m_city, -25.0, 25.0, r, l, &f);
	render_tile_at(m->m_null, -25.0, 50.0, r, l, &f);
	render_tile_at(m->m_city, -25.0, 75.0, r, l, &f);
	render_tile_at(m->m_null, -25.0, 100.0, r, l, &f);
	render_tile_at(m->m_null, -50.0, 75.0, r, l, &f);
	render_tile_at(m->m_null, -50.0, 100.0, r, l, &f);
	render_tile_at(m->m_null, 0.0, 25.0, r, l, &f);
	render_tile_at(m->m_city, 0.0, 50.0, r, l, &f);
	render_tile_at(m->m_null, 0.0, 75.0, r, l, &f);
	render_tile_at(m->m_null, 0.0, 100.0, r, l, &f);
#endif
	asset_file_render_end(m->m_assets);
}

void map_render(map_t m, renderer_t r, light_t l)
{
	render_map(m, r, l);
}

void map_get_size(map_t m, unsigned int *x, unsigned int *y)
{
	if ( x )
		*x = 5000;
	if ( y )
		*y = 2000;
}

map_t map_load(renderer_t r, const char *name)
{
	struct _map *m = NULL;

	m = calloc(1, sizeof(*m));
	if ( NULL == m )
		goto out;

	m->m_assets = asset_file_open("data/assets.db");
	if ( NULL == m->m_assets )
		goto out_free;

	m->m_null = tile_get(m->m_assets, "data/tiles/null");
	if ( NULL == m->m_null )
		goto out_free_assets;

	m->m_city = tile_get(m->m_assets, "data/tiles/city00");
	if ( NULL == m->m_city )
		goto out_free_null;

	m->m_mosque = tile_get(m->m_assets, "data/tiles/mosque00");
	if ( NULL == m->m_mosque)
		goto out_free_city;
#if 0
	m->m_buf = blob_from_file(name, &m->m_sz);
	if ( NULL == m->m_buf )
		goto out_free;
#endif

	/* success */
	goto out;
#if 0
out_free_blob:
	blob_free(m->m_buf, m->m_sz);
#endif
out_free_city:
	tile_put(m->m_city);
out_free_null:
	tile_put(m->m_null);
out_free_assets:
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
#if 0
		blob_free(m->m_buf, m->m_sz);
#endif
		tile_put(m->m_mosque);
		tile_put(m->m_city);
		tile_put(m->m_null);
		asset_file_close(m->m_assets);
		free(m);
	}
}
