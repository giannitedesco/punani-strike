#ifndef _TEXTURE_INTERNAL_H
#define _TEXTURE_INTERNAL_H

#include "list.h"

struct _texture {
	const char *t_name;
	unsigned int t_x;
	unsigned int t_y;
	unsigned int t_ref;
	SDL_Surface *t_surf;
};

void tex_get(struct _texture *tex);

SDL_Surface *tex_rgba(unsigned int x, unsigned int y);
SDL_Surface *tex_rgb(unsigned int x, unsigned int y);

#endif /* _TEXTURE_INTERNAL_H */
