/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_ASSET_H
#define _PUNANI_ASSET_H

typedef struct _asset_file *asset_file_t;
typedef struct _asset *asset_t;

asset_file_t asset_file_open(const char *fn);
asset_t asset_file_get(asset_file_t f, const char *name);
void asset_file_render_begin(asset_file_t f, renderer_t r, light_t l);
void asset_file_render_end(asset_file_t f);
void asset_file_close(asset_file_t f);

void asset_render(asset_t a, renderer_t r, light_t l);
void asset_render_bbox(asset_t a, renderer_t r);
void asset_put(asset_t a);
float asset_radius(asset_t a);
void asset_mins(asset_t a, vec3_t mins);
void asset_maxs(asset_t a, vec3_t maxs);

void assets_recalc_shadow_vols(light_t l);
void asset_file_dirty_shadows(asset_file_t f);

int asset_collide_line(asset_t a, const vec3_t p1, const vec3_t p2, vec3_t hit);
int asset_collide_sphere(asset_t a, const vec3_t c, float r, vec3_t hit);

#endif /* _PUNANI_ASSET_H */
