/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_RENDERER_H
#define _PUNANI_RENDERER_H

typedef struct _renderer *renderer_t;

#include <punani/tex.h>
#include <punani/game.h>

renderer_t renderer_new(game_t g);

/* get screen res */
void renderer_size(renderer_t r, unsigned int *x, unsigned int *y);

void renderer_exit(renderer_t r, int code);
int renderer_mode(renderer_t r, const char *title,
			unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen);
int renderer_main(renderer_t r);

void renderer_clear_color(renderer_t x, float r, float g, float b);
void renderer_rotate(renderer_t r, float deg, float x, float y, float z);
void renderer_translate(renderer_t r, float x, float y, float z);

void renderer_render_2d(renderer_t r);
void renderer_blit(renderer_t r, texture_t tex, prect_t *src, prect_t *dst);

void renderer_render_3d(renderer_t r);
void renderer_wireframe(renderer_t r, int wireframe);
void renderer_viewangles(renderer_t r, float pitch, float roll, float yaw);
void renderer_xlat_eye_to_obj(renderer_t r, vec3_t out, const vec3_t in);
void renderer_xlat_world_to_obj(renderer_t r, vec3_t out, const vec3_t in);
void renderer_unproject(renderer_t r, vec3_t out,
			unsigned int x, unsigned int y, float h);
void renderer_get_frustum_quad(renderer_t r, float h, vec3_t q[4]);

void renderer_free(renderer_t r);

#endif /* _PUNANI_RENDERER_H */
