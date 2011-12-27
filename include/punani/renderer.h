/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_RENDERER_H
#define _PUNANI_RENDERER_H

#include <punani/tex.h>

typedef struct _renderer *renderer_t;

void renderer_blit(renderer_t r, texture_t tex, SDL_Rect *src, SDL_Rect *dst);
void renderer_size(renderer_t r, unsigned int *x, unsigned int *y);
void renderer_exit(renderer_t r, int code);

#endif /* _PUNANI_RENDERER_H */
