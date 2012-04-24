/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/vec.h>
#include <punani/renderer.h>
#include <punani/punani_gl.h>
#include <punani/tex.h>
#include <punani/font.h>

#include <stdarg.h>

struct _font {
	texture_t f_tex;
	int f_lists;
};

#define UV_INC (1.0 / 16.0)
font_t font_load(renderer_t r, const char *fn)
{
	struct _font *f;
	unsigned int i;

	f = calloc(1, sizeof(*f));
	if ( NULL == f )
		goto out;

	f->f_tex = png_get_by_name(r, fn, 0);
	if ( NULL == f->f_tex )
		goto out_free;

	f->f_lists = glGenLists(0x100);
	texture_bind(f->f_tex);
	for (i = 0; i < 0x100; i++) {
		float cx = (i % 16) / 16.0f;
		float cy = (i / 16) / 16.0f;

		glNewList(f->f_lists + i, GL_COMPILE);
		glBegin(GL_QUADS);
		glTexCoord2f(cx, cy);
		glVertex2i(0, 0);
		glTexCoord2f(cx + UV_INC, cy);
		glVertex2i(16, 0);
		glTexCoord2f(cx + UV_INC, cy + UV_INC);
		glVertex2i(16, 16);
		glTexCoord2f(cx, cy + UV_INC);
		glVertex2i(0, 16);
		glEnd();
		glTranslated(14, 0, 0);
		glEndList();
	}

	/* success */
	goto out;

out_free:
	free(f);
	f = NULL;
out:
	return f;
}

void font_print(font_t f, unsigned int x, unsigned int y, const char *str)
{
	float env_color[4];

	env_color[0] = 1.0;
	env_color[1] = 1.0;
	env_color[2] = 1.0;
	env_color[3] = 0.1;

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env_color);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glColor4f(0.0, 0.0, 0.0, 1.0);

	glPushMatrix();
	glTranslated(x, y, 0);
	glListBase(f->f_lists);
	texture_bind(f->f_tex);
	glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
	glPopMatrix();
}

void font_printf(font_t f, unsigned int x, unsigned int y, const char *fmt, ...)
{
	static char *abuf;
	static size_t abuflen;
	int len;
	va_list va;
	char *new;

again:
	va_start(va, fmt);

	len = vsnprintf(abuf, abuflen, fmt, va);
	if ( len < 0 ) /* bug in old glibc */
		len = 0;
	if ( (size_t)len < abuflen )
		goto done;

	new = realloc(abuf, len + 1);
	if ( new == NULL )
		goto done;

	abuf = new;
	abuflen = len + 1;
	goto again;

done:
	font_print(f, x, y, abuf);
	va_end(va);
}

void font_free(font_t f)
{
	if ( f ) {
		glDeleteLists(f->f_lists, 0x100);
		free(f);
	}
}
