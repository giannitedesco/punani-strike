/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/asset.h>
#include <punani/tile.h>
#include <punani/blob.h>

#include <GL/gl.h>

#include "list.h"

#define TILE_INTERNAL 1
#include "tilefile.h"
#include "dessert-stroke.h"

void tile_render(tile_t t, renderer_t r, light_t l)
{
	unsigned int i;

#if 1
	if ( NULL == l ) {
		glColor4f(0.4, 0.4, 0.4, 1.0);
		glBegin(GL_QUAD_STRIP);
		glNormal3f(0.0, 1.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, TILE_Y);
		glVertex3f(TILE_X, 0.0, 0.0);
		glVertex3f(TILE_X, 0.0, TILE_Y);
		glEnd();
	}
#endif

	for(i = 0; i < t->t_num_items; i++) {
		struct _item *item = t->t_items + i;
		glPushMatrix();
		renderer_translate(r, item->x, 0.0, item->y);
		glColor4f(0.3, 0.3, 0.3, 1.0);
		asset_render(item->asset, r, l);
		glPopMatrix();
	}
}
