/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _TEXTURE_INTERNAL_H
#define _TEXTURE_INTERNAL_H

#include "list.h"

struct _texture {
	const char *t_name;
	unsigned int t_x;
	unsigned int t_y;
	unsigned int t_ref;
	const struct tex_ops *t_ops;
	void (*t_dtor)(struct _texture *tex);
	union {
		struct {
			void *surf;
		}sdl;
		struct {
			uint8_t *buf;
			unsigned int texnum;
			unsigned int height, width;
			int format;
			unsigned int uploaded;
		}gl;
	}t_u;
};

struct tex_ops {
	int (*alloc_rgba)(struct _texture *t, unsigned int x, unsigned int y);
	int (*alloc_rgb)(struct _texture *t, unsigned int x, unsigned int y);
	void (*lock)(struct _texture *t);
	uint8_t *(*pixels)(struct _texture *t);
	void (*unlock)(struct _texture *t);
	void (*free)(struct _texture *t);
};

void tex_get(struct _texture *tex);

void tex_init(struct _texture *t, struct _renderer *r);
int tex_alloc_rgba(struct _texture *tex, unsigned int x, unsigned int y);
int tex_alloc_rgb(struct _texture *tex, unsigned int x, unsigned int y);
void tex_lock(struct _texture *tex);
void tex_unlock(struct _texture *tex);
void tex_free(struct _texture *tex);
uint8_t *tex_pixels(struct _texture *t);

#endif /* _TEXTURE_INTERNAL_H */
