/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/map.h>
#include <punani/blob.h>

#include <GL/gl.h>

struct _map {
	uint8_t *buf;
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
	/* look down on things */
	glRotatef(30.0f, 1, 0, 0);
	glRotatef(45.0f, 0, 1, 0);
	glTranslatef(0.0, -30, 0.0);

	glTranslatef(0.0, 0.0, -30.0);
	drawHighrise();

	glTranslatef(15.0, 0.0, 0.0);
	drawHighrise();

	glTranslatef(15.0, 0.0, 0.0);
	drawHighrise();
}

void map_get_size(map_t map, unsigned int *x, unsigned int *y)
{
	if ( x )
		*x = 5000;
	if ( y )
		*y = 2000;
}

map_t map_load(renderer_t r, const char *name)
{
	struct _map *map = NULL;

	map = calloc(1, sizeof(*map));
	if ( NULL == map )
		goto out;

	map->buf = blob_from_file(name, &map->sz);
//	if ( NULL == map->buf )
//		goto out_free;

	/* success */
	goto out;
//out_free_blob:
//	blob_free(map->buf, map->sz);
//out_free:
//	free(map);
//	map = NULL;
out:
	return map;
}

void map_free(map_t map)
{
	if ( map ) {
		blob_free(map->buf, map->sz);
		free(map);
	}
}
