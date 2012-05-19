/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_TILE_H
#define _PUNANI_TILE_H

typedef struct _tile *tile_t;

tile_t tile_get(asset_file_t f, const char *fn);
void tile_render(tile_t t, renderer_t r, light_t l);
void tile_render_bbox(tile_t t, renderer_t r);
void tile_put(tile_t t);

int tile_collide_line(tile_t t, const vec3_t a, const vec3_t b, vec3_t hit);

struct tile_hit {
	struct _asset *asset;
	vec3_t origin; /* in tile local space */
	vec2_t times; /* only valid for sweep tests */
	unsigned int index;
};
typedef int (*tile_cbfn_t)(const struct tile_hit *hit, void *priv);

int tile_collide_sphere(tile_t t, const vec3_t c, float r,
			tile_cbfn_t cb, void *priv);
int tile_sweep(tile_t t, const struct obb *sweep,
			tile_cbfn_t cb, void *priv);

#endif /* _PUNANI_TILE_H */
