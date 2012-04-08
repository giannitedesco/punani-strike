/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/asset.h>
#include <punani/tile.h>
#include <punani/blob.h>

#include <GL/gl.h>

#include "list.h"

#define TILE_INTERNAL 1
#include "tilefile.h"

void tile_render(tile_t t)
{
	unsigned int i;
	for(i = 0; i < t->t_num_items; i++) {
		struct _item *item = t->t_items + i;
		glPushMatrix();
		glTranslatef(-item->x, 0.0, -item->y);
		asset_render(item->asset);
		glPopMatrix();
	}
}
