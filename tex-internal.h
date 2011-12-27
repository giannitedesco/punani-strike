/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _TEXTURE_INTERNAL_H
#define _TEXTURE_INTERNAL_H

#include "list.h"
#include <SDL.h>

struct _texture {
	const char *t_name;
	unsigned int t_x;
	unsigned int t_y;
	unsigned int t_ref;
	SDL_Surface *t_surf;
	void (*dtor)(struct _texture *tex);
};

void tex_get(struct _texture *tex);

SDL_Surface *tex_rgba(unsigned int x, unsigned int y);
SDL_Surface *tex_rgb(unsigned int x, unsigned int y);
SDL_Surface *texture_surface(texture_t tex);

#endif /* _TEXTURE_INTERNAL_H */
