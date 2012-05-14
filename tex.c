/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/game.h>
#include <punani/tex.h>
#include <punani/punani_gl.h>
#include "tex-internal.h"

void tex_init(struct _texture *t)
{
	memset(t, 0, sizeof(*t));
}

void tex_get(struct _texture *t)
{
	t->t_ref++;
}

int tex_alloc_rgba(struct _texture *t, unsigned int x, unsigned int y)
{
	t->gl.buf = malloc(x * y * 4);
	if ( NULL == t->gl.buf )
		return 0;
	t->gl.width = x;
	t->gl.height = y;
	t->gl.format = GL_RGBA;
	return 1;
}

int tex_alloc_rgb(struct _texture *t, unsigned int x, unsigned int y)
{
	t->gl.buf = malloc(x * y * 3);
	if ( NULL == t->gl.buf )
		return 0;
	t->gl.width = x;
	t->gl.height = y;
	t->gl.format = GL_RGB;
	return 1;
}

void tex_lock(struct _texture *t)
{
}

void tex_unlock(struct _texture *t)
{
}

static void tex_unbind(struct _texture *tex)
{
	if ( tex->gl.uploaded ) {
		glDeleteTextures(1, &tex->gl.texnum);
		tex->gl.uploaded = 0;
	}
}

static void tex_upload(struct _texture *tex)
{
	glBindTexture(GL_TEXTURE_2D, tex->gl.texnum);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		tex->gl.width,
		tex->gl.height,
		0,
		tex->gl.format,
		GL_UNSIGNED_BYTE,
		tex->gl.buf);
}

void texture_bind(texture_t tex)
{
	if ( !tex->gl.uploaded ) {
		glGenTextures(1, &tex->gl.texnum);
		tex_upload(tex);
		tex->gl.uploaded = 1;
	}
	glBindTexture(GL_TEXTURE_2D, tex->gl.texnum);
}

void tex_free(struct _texture *t)
{
	if ( t->gl.buf ) {
		tex_unbind(t);
		free(t->gl.buf);
		t->gl.buf = NULL;
	}
}

uint8_t *tex_pixels(struct _texture *t)
{
	return t->gl.buf;
}

void texture_put(texture_t t)
{
	if ( t ) {
		t->t_ref--;
		if ( !t->t_ref ) {
			tex_free(t);
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
