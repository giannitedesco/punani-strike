/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/vec.h>
#include <punani/punani_gl.h>
#include <punani/particles.h>
#include "list.h"
#include "hgang.h"

struct plerp {
	vec3_t pos;
	vec4_t color;
};

struct particle {
	struct particle *next;
	struct plerp cur;
	struct plerp old;
	vec3_t velocity;
	unsigned int lifetime;
};

/* particles system */
struct _particles {
	struct list_head p_list;
	struct particle *p_active;
	hgang_t p_mem;
};

static LIST_HEAD(particles);

particles_t particles_new(unsigned int max)
{
	struct _particles *p;

	p = calloc(1, sizeof(*p));
	if ( NULL == p )
		goto out;

	p->p_mem = hgang_new(sizeof(*p->p_active), max);
	if ( NULL == p->p_mem )
		goto out_free;

	/* success */
	list_add_tail(&p->p_list, &particles);
	goto out;

out_free:
	free(p);
	p = NULL;
out:
	return p;
}

static void particle_tick(struct particle *pp)
{
	v_copy(pp->old.pos, pp->cur.pos);
	v_add(pp->cur.pos, pp->velocity);
}

void particles_think(particles_t p)
{
	struct particle *arr, *tmp, *pp;

	if ( NULL == p->p_active )
		return;

	arr = p->p_active;
	p->p_active = NULL;

	for(pp = arr, tmp = pp->next; pp; pp = tmp, tmp = (pp) ? pp->next : NULL) {
		if ( !pp->lifetime ) {
			hgang_return(p->p_mem, pp);
			continue;
		}else{
			pp->next = p->p_active;
			p->p_active = pp;
			pp->lifetime--;
			particle_tick(pp);
		}
	}
}

static void particles_render(particles_t p, float lerp)
{
	struct particle *pp;

//	glEnable(GL_POINT_SPRITE);
	glDepthMask(GL_FALSE);
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glBegin(GL_POINTS);
	for(pp = p->p_active; pp; pp = pp->next) {
		vec3_t pos;

		pos[0] = pp->old.pos[0] + pp->velocity[0] * lerp;
		pos[1] = pp->old.pos[1] + pp->velocity[1] * lerp;
		pos[2] = pp->old.pos[2] + pp->velocity[2] * lerp;

		glColor4fv((GLfloat *)pp->cur.color);
		glVertex3fv((GLfloat *)pos);
	}
	glEnd();
	glEnable(GL_LIGHTING);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
//	glDisable(GL_POINT_SPRITE);
}

void particles_free(particles_t p)
{
	if ( p ) {
		list_del(&p->p_list);
		hgang_free(p->p_mem);
		free(p);
	}
}

void particles_render_all(float lerp)
{
	struct _particles *p;

	list_for_each_entry(p, &particles, p_list) {
		particles_render(p, lerp);
	}
}

void particles_think_all(void)
{
	struct _particles *p;

	list_for_each_entry(p, &particles, p_list) {
		particles_think(p);
	}
}

static float crand(void)
{
	return (rand() & 32767) * (2.0/32767) - 1;
}

/* shamelessly ripped from quake2 */
void particles_emit(particles_t p, const vec3_t begin, const vec3_t end)
{
	struct particle *pp;
	unsigned int i;
	vec3_t move;
	vec3_t vec;
	float len, dec;

	v_copy(move, begin);
	v_sub(vec, end, begin);
	len = v_normlen(vec);

	dec = 0.1;
	v_scale(vec, dec);

	while(len > 0) {
		float shade;
		len -= dec;

		pp = hgang_alloc0(p->p_mem);
		if ( NULL == pp )
			return;

		pp->next = p->p_active;
		p->p_active = pp;

		pp->lifetime = 40;
		shade = crand() * 0.4;
		pp->cur.color[0] = 0.6 + shade;
		pp->cur.color[1] = 0.6 + shade;
		pp->cur.color[2] = 0.6 + shade;
		pp->cur.color[3] = 1.0;

		for(i = 0; i < 3; i++) {
			pp->cur.pos[i] = move[i] + crand();
			pp->velocity[i] = crand() * 0.2;
		}
		particle_tick(pp);
		v_add(move, vec);
	}
}
