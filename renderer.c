/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/tex.h>
#include <punani/renderer.h>
#include "render-internal.h"

void renderer_blit(renderer_t r, texture_t tex, prect_t *src, prect_t *dst)
{
	r->ops->blit(r->priv, tex, src, dst);
}

void renderer_size(renderer_t r, unsigned int *x, unsigned int *y)
{
	r->ops->size(r->priv, x, y);
}

void renderer_exit(renderer_t r, int code)
{
	r->ops->exit(r->priv, code);
}

int renderer_init(renderer_t r, unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen)
{
	return r->ops->init(r->priv, x, y, depth, fullscreen);
}
