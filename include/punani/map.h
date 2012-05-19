/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_MAP_H
#define _PUNANI_MAP_H

typedef struct _map *map_t;

map_t map_load(renderer_t r, const char *name);
void map_get_size(map_t map, unsigned int *x, unsigned int *y);
void map_render(map_t map, renderer_t r, light_t l);
int map_save(map_t map, const char *fn);
int map_collide_line(map_t map, const vec3_t a, const vec3_t b, vec3_t hit);
void map_free(map_t map);

struct map_hit {
	struct _asset *asset; /* mesh */
	struct _tile *tile;
	vec3_t origin; /* of asset, in world space */
	vec2_t times; /* only valid for sweep tests */
	unsigned int map_x, map_y; /* 2d tile coords */
	unsigned int tile_idx; /* item index in tile */
};
typedef int (*map_cbfn_t)(const struct map_hit *hit, void *priv);
int map_findradius(map_t map, const vec3_t c, float r,
			map_cbfn_t cb, void *priv);
int map_sweep(map_t map, const struct obb *obb,
			map_cbfn_t cb, void *priv);

#endif /* _PUNANI_MAP_H */
