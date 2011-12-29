/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_MAP_H
#define _PUNANI_MAP_H

typedef struct _map *map_t;

map_t map_load(const char *name);
void map_get_size(map_t map, unsigned int *x, unsigned int *y);
void map_render(map_t map, renderer_t r, prect_t *src);
texture_t map_get_tiles(map_t map, unsigned int *x, unsigned int *y);
int map_tile_at(map_t map, unsigned int x, unsigned int y);
void map_set_tile_at(map_t map, unsigned int x, unsigned int y, int id);
int map_save(map_t map, const char *fn);
void map_free(map_t map);

#endif /* _PUNANI_MAP_H */
