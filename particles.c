/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/blob.h>
#include <punani/punani_gl.h>
#include <punani/particles.h>

struct plerp {
	vec3_t pos;
	vec4_t color;
};

struct particle {
	struct plerp cur;
	struct plerp old;
	vec3_t velocity;
	unsigned int age;
};

/* particles system */
struct _particles {
	unsigned int p_max;
	unsigned int p_num;
	struct particle p_elem[0];
};

particles_t particles_new(unsigned int max)
{
	struct _particles *p;

	p = calloc(1, sizeof(*p) + sizeof(*p->p_elem) * max);
	if ( NULL == p )
		goto out;

	p->p_max = max;
out:
	return p;
}

void particles_think(particles_t p)
{
}

void particles_render(particles_t p, float lerp)
{
	unsigned int i;
	glEnable(GL_POINT_SPRITE);
	for(i = 0; i < p->p_num; i++) {
		struct particle *pp = p->p_elem + i;
		vec3_t pos;

		pos[0] = pp->old.pos[0] + pp->velocity[0] * lerp;
		pos[1] = pp->old.pos[1] + pp->velocity[1] * lerp;
		pos[2] = pp->old.pos[2] + pp->velocity[2] * lerp;

		glBegin(GL_POINTS);
		glVertex3fv((GLfloat *)pos);
		glEnd();
	}
	glDisable(GL_POINT_SPRITE);
}

void particles_free(particles_t p)
{
	if ( p ) {
		free(p);
	}
}
