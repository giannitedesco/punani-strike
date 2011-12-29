/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_RENDERER_H
#define _PUNANI_RENDERER_H

#include <punani/tex.h>

typedef struct _renderer *renderer_t;

void renderer_blit(renderer_t r, texture_t tex, prect_t *src, prect_t *dst);
void renderer_size(renderer_t r, unsigned int *x, unsigned int *y);
void renderer_exit(renderer_t r, int code);
int renderer_mode(renderer_t r, const char *title,
			unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen);
int renderer_main(renderer_t r);

#endif /* _PUNANI_RENDERER_H */
