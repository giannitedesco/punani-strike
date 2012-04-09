/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/map.h>
#include <punani/asset.h>
#include <punani/tile.h>
#include <punani/blob.h>

#include <GL/gl.h>

struct _map {
	asset_file_t m_assets;
	tile_t m_tile;
	tile_t m_null;
#if 0
	uint8_t *m_buf;
	size_t m_sz;
#endif
};

static void render_tile_at(tile_t t, float x, float y, renderer_t r)
{
	glPushMatrix();
	renderer_translate(r, -x, 0.0, -y);
	tile_render(t, r);
	glPopMatrix();
}

void map_render(map_t m, renderer_t r)
{
	asset_file_render_begin(m->m_assets);

	glColor4f(0.5, 0.5, 0.5, 1.0);
	render_tile_at(m->m_null, -25.0, 50.0, r);
	render_tile_at(m->m_null, 0.0, 25.0, r);
	render_tile_at(m->m_null, 0.0, 50.0, r);
	render_tile_at(m->m_tile, -25.0, 25.0, r);

	asset_file_render_end(m->m_assets);
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

	m->m_tile = tile_get(m->m_assets, "data/tiles/city00");
	if ( NULL == m->m_tile )
		goto out_free_null;
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
		tile_put(m->m_tile);
		tile_put(m->m_null);
		asset_file_close(m->m_assets);
		free(m);
	}
}
