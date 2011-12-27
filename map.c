/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/game.h>
#include <punani/tex.h>
#include <punani/map.h>

struct _map {
	texture_t tiles;
};

void map_render(map_t map, game_t g, SDL_Rect *src)
{
	game_blit(g, map->tiles, src, NULL);
}

void map_get_size(map_t map, unsigned int *x, unsigned int *y)
{
	if ( x )
		*x = texture_width(map->tiles);
	if ( y )
		*y = texture_height(map->tiles);
}

map_t map_load(const char *name)
{
	struct _map *map = NULL;

	map = calloc(1, sizeof(*map));
	if ( NULL == map )
		goto out;

	map->tiles = png_get_by_name(name, 0);
	if ( NULL == map->tiles )
		goto out_free;

	/* success */
	goto out;
out_free:
	free(map);
	map = NULL;
out:
	return map;
}

void map_free(map_t map)
{
	if ( map ) {
		texture_put(map->tiles);
		free(map);
	}
}
