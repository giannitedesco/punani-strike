/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/map.h>
#include <punani/blob.h>

#include "mapfile.h"

#include <GL/gl.h>

struct _map {
	texture_t tiles;
	unsigned int tiles_per_row;
	struct mapfile_hdr *hdr;
	uint16_t *rmap;
	size_t sz;
};

static void drawHighrise(void)
{
	glBegin(GL_QUADS);
	/* back */
	glNormal3f(0.0, 0.0, 1.0);  /* constant normal for side */
	glVertex3f(5.0, 0.0, 5.0);
	glVertex3f(5.0, 20.0, 5.0);
	glVertex3f(-5.0, 20.0, 5.0);
	glVertex3f(-5.0, 0.0, 5.0);

	/* right */
	glNormal3f(-1.0, 0.0, 0.0);  /* constant normal for side */
	glVertex3f(-5.0, 0.0, 5.0);
	glVertex3f(-5.0, 20.0, 5.0);
	glVertex3f(-5.0, 20.0, -5.0);
	glVertex3f(-5.0, 0.0, -5.0);

	/* front */
	glNormal3f(0.0, 0.0, -1.0);  /* constant normal for side */
	glVertex3f(-5.0, 0.0, -5.0);
	glVertex3f(-5.0, 20.0, -5.0);
	glVertex3f(5.0, 20.0, -5.0);
	glVertex3f(5.0, 0.0, -5.0);

	/* left */
	glNormal3f(1.0, 0.0, 0.0);  /* constant normal for side */
	glVertex3f(5.0, 0.0, -5.0);
	glVertex3f(5.0, 20.0, -5.0);
	glVertex3f(5.0, 20.0, 5.0);
	glVertex3f(5.0, 0.0, 5.0);

	/* cap */
	glNormal3f(0.0, 1.0, 0.0);  /* constant normal for side */
	glVertex3f(5.0, 20.0, -5.0);
	glVertex3f(-5.0, 20.0, -5.0);
	glVertex3f(-5.0, 20.0, 5.0);
	glVertex3f(5.0, 20.0, 5.0);
	glEnd();
}

void map_render(map_t map, renderer_t r, prect_t *scr)
{
	glTranslatef(0.0, 0.0, -30.0);
	drawHighrise();

	glTranslatef(-15.0, 0.0, 0.0);
	drawHighrise();

	glTranslatef(30.0, 0.0, 0.0);
	drawHighrise();
}

void map_get_size(map_t map, unsigned int *x, unsigned int *y)
{
	if ( x )
		*x = map->hdr->xtiles * map->hdr->tile_width;
	if ( y )
		*y = map->hdr->ytiles * map->hdr->tile_height;
}

static int sanity_check(struct _map *map)
{
	if ( map->sz < sizeof(*map->hdr) )
		return 0;
	if ( map->hdr->magic != MAPFILE_MAGIC )
		return 0;
	if ( (sizeof(*map->hdr) + (map->hdr->xtiles *
					map->hdr->ytiles *
					sizeof(uint16_t))) < map->sz )
		return 0;

	return 1;
}

map_t map_load(renderer_t r, const char *name)
{
	struct _map *map = NULL;
	char fn[512];
	uint8_t *buf;

	map = calloc(1, sizeof(*map));
	if ( NULL == map )
		goto out;

	buf = blob_from_file(name, &map->sz);
	if ( NULL == buf )
		goto out_free;

	map->hdr = (struct mapfile_hdr *)buf;
	map->rmap = (uint16_t *)(buf + sizeof(*map->hdr));
	if ( !sanity_check(map) )
		goto out_free_blob;

	snprintf(fn, sizeof(fn), "data/tiles/%s", map->hdr->tilemap);
	map->tiles = png_get_by_name(r, fn, 0);
	if ( NULL == map->tiles )
		goto out_free_blob;

	map->tiles_per_row = texture_width(map->tiles) / map->hdr->tile_width;

	/* success */
	goto out;
out_free_blob:
	blob_free((uint8_t *)map->hdr, map->sz);
out_free:
	free(map);
	map = NULL;
out:
	return map;
}

void map_free(map_t map)
{
	if ( map ) {
		blob_free((uint8_t *)map->hdr, map->sz);
		texture_put(map->tiles);
		free(map);
	}
}

int map_save(map_t map, const char *fn)
{
	return blob_to_file((uint8_t *)map->hdr, map->sz, fn);
}
