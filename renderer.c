/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/game.h>
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

int renderer_mode(renderer_t r, const char *title,
			unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen)
{
	return r->ops->mode(r->priv, title, x, y, depth, fullscreen);
}

int renderer_main(renderer_t r)
{
	return r->ops->main(r->priv);
}

int renderer_ctor(struct _renderer *r, const struct render_ops *rops,
			struct _game *g)
{
	r->ops = rops;
	if ( !r->ops->ctor(r, g) )
		return 0;
	return 1;
}

void renderer_dtor(struct _renderer *r)
{
	r->ops->dtor(r->priv);
	r->priv = NULL;
}

extern const struct render_ops render_gl;

static const struct render_ops *rop_lookup(const char *name)
{
	static const struct {
		const char *name;
		const struct render_ops *rops;
	}rlist[] = {
		{"GL", &render_gl},
	};
	unsigned int i;

	for(i = 0; i < sizeof(rlist)/sizeof(*rlist); i++) {
		if ( !strcmp(rlist[i].name, name) )
			return rlist[i].rops;
	}

	return NULL;
}

renderer_t renderer_by_name(const char *name, struct _game *g)
{
	const struct render_ops *rop;
	struct _renderer *r;

	rop = rop_lookup(name);
	if ( NULL == rop )
		return NULL;

	r = calloc(1, sizeof(*r));
	if ( NULL == r )
		return NULL;

	r->ops = rop;

	if ( !r->ops->ctor(r, g) ) {
		free(r);
		return NULL;
	}

	return r;
}

void renderer_free(renderer_t r)
{
}
