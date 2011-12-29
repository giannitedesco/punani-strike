/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _RENDERER_INTERNAL_H
#define _RENDERER_INTERNAL_H

struct render_ops {
	void (*blit)(void *priv, texture_t tex, prect_t *src, prect_t *dst);
	void (*size)(void *priv, unsigned int *x, unsigned int *y);
	void (*exit)(void *priv, int code);
	int  (*mode)(void *priv, const char *title,
			unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen);
	int  (*main)(void *priv);
	int  (*ctor)(struct _renderer *r, struct _game *g);
	void (*dtor)(void *priv);
};

struct _renderer {
	const struct render_ops *ops;
	void *priv;
};

int renderer_ctor(struct _renderer *r, const struct render_ops *rops,
			struct _game *g);
void renderer_dtor(struct _renderer *r);
renderer_t renderer_by_name(const char *name, struct _game *g);
void renderer_free(renderer_t r);


#endif /* _RENDERER_INTERNAL_H */
