/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_MAP_H
#define _PUNANI_MAP_H

typedef struct _map *map_t;

map_t map_load(const char *name);
void map_get_size(map_t map, unsigned int *x, unsigned int *y);
void map_render(map_t map, renderer_t r, SDL_Rect *src);
void map_free(map_t map);

#endif /* _PUNANI_MAP_H */
