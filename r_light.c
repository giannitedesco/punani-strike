/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/punani_gl.h>

#include <math.h>

#define RENDER_LIGHTS 1
#include "render-internal.h"

struct _light {
	renderer_t owner;
	uint8_t num;
	uint8_t enabled;
	float pos[4];
	float color[4];
};

light_t light_new(renderer_t r)
{
	struct _light *l = NULL;
	int num;

	num = renderer_get_free_light(r);
	if ( num < 0 )
		goto out;

	l = calloc(1, sizeof(*l));
	if ( NULL == l )
		goto out;

	l->owner = r;
	l->num = num;

	l->pos[0] = -25.0;
	l->pos[1] = 20.0;
	l->pos[2] = 25.0;
	l->pos[3] = 0.0;

	l->color[0] = 1.0;
	l->color[1] = 0.8;
	l->color[2] = 0.6;
	l->color[3] = 1.0;

	l->enabled = 1;

	renderer_set_light(r, num, l);
out:
	return l;
}

void light_set_pos(light_t l, const vec3_t pos)
{
	l->pos[0] = pos[0];
	l->pos[1] = pos[1];
	l->pos[2] = pos[2];
}

void light_get_pos(light_t l, vec3_t pos)
{
	pos[0] = l->pos[0];
	pos[1] = l->pos[1];
	pos[2] = l->pos[2];
}

void light_disable(light_t l)
{
	l->enabled = 0;
}

void light_enable(light_t l)
{
	l->enabled = 1;
}

int light_enabled(light_t l)
{
	return l->enabled;
}

void light_render(light_t l)
{
	GLint num;

	num = GL_LIGHT0 + l->num;
	glEnable(num);
	glLightf(num, GL_CONSTANT_ATTENUATION, 1.0);
	glLightf(num, GL_LINEAR_ATTENUATION, 0.0);
	glLightf(num, GL_QUADRATIC_ATTENUATION, 0.0);
	glLightfv(num, GL_DIFFUSE, l->color);
	glLightfv(num, GL_POSITION, l->pos);
}

void light_free(light_t l)
{
	if ( l ) {
		renderer_nuke_light(l->owner, l->num);
		free(l);
	}
}
