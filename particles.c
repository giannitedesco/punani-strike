/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/vec.h>
#include <punani/punani_gl.h>
#include <punani/particles.h>
#include <punani/cvar.h>
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
	texture_t p_sprite;
	struct list_head p_list;
	struct particle *p_active;
	hgang_t p_mem;
	unsigned int p_ref;
};

static LIST_HEAD(particles);

static unsigned int var_points = 1;
static unsigned int var_point_sprites = 1;

particles_t particles_new(renderer_t r, unsigned int max)
{
	struct _particles *p;
	static int done;

	if ( !done ) {
		cvar_register_uint("particles",
					"points", &var_points);
		cvar_register_uint("particles",
					"sprites", &var_point_sprites);
		done = 1;
	}

	p = calloc(1, sizeof(*p));
	if ( NULL == p )
		goto out;

	p->p_mem = hgang_new(sizeof(*p->p_active), max);
	if ( NULL == p->p_mem )
		goto out_free;

	p->p_sprite = png_get_by_name(r, "data/smoke.png");
	if ( NULL == p->p_sprite )
		goto out_free_mem;

	/* success */
	list_add_tail(&p->p_list, &particles);
	p->p_ref = 1;
	goto out;

out_free_mem:
	hgang_free(p->p_mem);
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
	pp->cur.color[3] *= 0.950;
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
			p->p_ref--;
			continue;
		}else{
			pp->next = p->p_active;
			p->p_active = pp;
			pp->lifetime--;
			particle_tick(pp);
		}
	}

	if ( !p->p_ref )
		particles_free(p);
}

static void particles_render(particles_t p, renderer_t r, float lerp)
{
	struct particle *pp;
	float scale = 2.0;

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);

	if ( !var_points || var_point_sprites ) {
		glEnable(GL_TEXTURE_2D);
		texture_bind(p->p_sprite);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	if ( var_points ) {
		if ( var_point_sprites ) {
			glTexEnvi(GL_POINT_SPRITE,
					GL_COORD_REPLACE, GL_TRUE);
			glEnable(GL_POINT_SPRITE);
			glPointSize(16.0);
		}else{
			glPointSize(4.0);
		}
		glBegin(GL_POINTS);
	}

	for(pp = p->p_active; pp; pp = pp->next) {
		if ( var_points ) {
			vec3_t pos;

			pos[0] = pp->old.pos[0] + pp->velocity[0] * lerp;
			pos[1] = pp->old.pos[1] + pp->velocity[1] * lerp;
			pos[2] = pp->old.pos[2] + pp->velocity[2] * lerp;

			glColor4fv((GLfloat *)pp->cur.color);
			glVertex3fv((GLfloat *)pos);
		}else{
			vec3_t pos, angles;

			pos[0] = pp->old.pos[0] + pp->velocity[0] * lerp;
			pos[1] = pp->old.pos[1] + pp->velocity[1] * lerp;
			pos[2] = pp->old.pos[2] + pp->velocity[2] * lerp;

			glPushMatrix();
			renderer_translate(r, pos[0], pos[1], pos[2]);
			renderer_get_viewangles(r, angles);
			renderer_rotate(r, -angles[0], 1.0, 0.0, 0.0);
			renderer_rotate(r, -angles[1], 0.0, 1.0, 0.0);
			renderer_rotate(r, -angles[2], 0.0, 0.0, 1.0);
			glBegin(GL_QUADS);
			glColor4fv((GLfloat *)pp->cur.color);

			glTexCoord2f(0.0, 0.0);
			glVertex3f(-scale, -scale, 0.0);

			glTexCoord2f(1.0, 0.0);
			glVertex3f(scale, -scale, 0.0);

			glTexCoord2f(1.0, 1.0);
			glVertex3f(scale, scale, 0.0);

			glTexCoord2f(0.0, 1.0);
			glVertex3f(-scale, scale, 0.0);

			glEnd();
			glPopMatrix();
		}
	}

	if ( var_points ) {
		glEnd();
		glDisable(GL_POINT_SPRITE);
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

void particles_free(particles_t p)
{
	if ( p ) {
		list_del(&p->p_list);
		hgang_free(p->p_mem);
		texture_put(p->p_sprite);
		free(p);
	}
}

void particles_unref(particles_t p)
{
	p->p_ref--;
	if ( !p->p_ref ) {
		particles_free(p);
	}
}

void particles_render_all(renderer_t r, float lerp)
{
	struct _particles *p;

	list_for_each_entry(p, &particles, p_list) {
		particles_render(p, r, lerp);
	}
}

void particles_think_all(void)
{
	struct _particles *p, *tmp;

	list_for_each_entry_safe(p, tmp, &particles, p_list) {
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

	dec = 0.3;
	v_scale(vec, dec);

	while(len > 0) {
		float shade;
		len -= dec;

		pp = hgang_alloc0(p->p_mem);
		if ( NULL == pp )
			return;

		pp->next = p->p_active;
		p->p_active = pp;
		p->p_ref++;

		pp->lifetime = 40;
		shade = crand() * 0.4;
		pp->cur.color[0] = 0.6 + shade;
		pp->cur.color[1] = 0.6 + shade;
		pp->cur.color[2] = 0.6 + shade;
		pp->cur.color[3] = 0.2;

		for(i = 0; i < 3; i++) {
			pp->cur.pos[i] = move[i] + crand();
			pp->velocity[i] = crand() * 0.2;
		}
		particle_tick(pp);
		v_add(move, vec);
	}
}
