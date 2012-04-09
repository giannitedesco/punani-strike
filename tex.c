/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/game.h>
#include "render-internal.h"
#include "tex-internal.h"

void tex_init(struct _texture *t, struct _renderer *r)
{
	memset(t, 0, sizeof(*t));
	t->t_ops = renderer_texops(r);
}

void tex_get(struct _texture *t)
{
	t->t_ref++;
}

int tex_alloc_rgba(struct _texture *t, unsigned int x, unsigned int y)
{
	return (*t->t_ops->alloc_rgba)(t, x, y);
}

int tex_alloc_rgb(struct _texture *t, unsigned int x, unsigned int y)
{
	return (*t->t_ops->alloc_rgb)(t, x, y);
}

void tex_lock(struct _texture *t)
{
	if ( t->t_ops->lock)
		(*t->t_ops->lock)(t);
}

void tex_unlock(struct _texture *t)
{
	if ( t->t_ops->unlock)
		(*t->t_ops->unlock)(t);
}

void tex_free(struct _texture *t)
{
	(*t->t_ops->free)(t);
}

uint8_t *tex_pixels(struct _texture *t)
{
	return (*t->t_ops->pixels)(t);
}

void texture_put(texture_t t)
{
	if ( t ) {
		t->t_ref--;
		if ( !t->t_ref ) {
			(*t->t_ops->free)(t);
			(*t->t_dtor)(t);
		}
	}
}

unsigned int texture_width(texture_t t)
{
	return t->t_x;
}

unsigned int texture_height(texture_t t)
{
	return t->t_y;
}
