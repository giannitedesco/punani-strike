/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/vec.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/asset.h>
#include <punani/chopper.h>
#include <punani/punani_gl.h>
#include <math.h>
#include "list.h"

#define VELOCITY_INCREMENTS	7
#define VELOCITY_UNIT		1

#define ANGLE_INCREMENT		((M_PI * 2.0) / 18)

struct missile {
	struct list_head m_list;
	asset_t m_mesh;
	vec3_t m_origin;
	vec3_t m_oldorigin;
	float m_velocity;
	float m_heading;
	unsigned int m_lifetime;
};

struct _chopper {
	asset_file_t asset;
	asset_file_t rotor_asset;
	asset_t fuselage;
	asset_t glass;
	asset_t rotor;

	struct list_head missiles;

	/* these ones are calculated based on "physics"
	 * vs. control surfaces and are the final answer
	 * as to what we will render in the next pass
	*/
	float x;
	float y;
	float oldx;
	float oldy;
	float oldlerp;
	unsigned int input;
	int bank;

	unsigned int last_fire;

	int throttle_time; /* in frames */
	float velocity; /* in pixels per frame */
	float heading; /* in radians */
	float avelocity;
	float oldvelocity;
	float oldheading;
};

void chopper_get_pos(chopper_t chopper, float *x, float *y, float lerp)
{
	if ( x )
		*x = chopper->oldx -
			(chopper->velocity * lerp) * sin(chopper->oldheading);
	if ( y )
		*y = chopper->oldy -
			(chopper->velocity * lerp) * cos(chopper->oldheading);
}

static chopper_t get_chopper(const char *file,
				float x, float y, float h)
{
	struct _chopper *c = NULL;
	asset_file_t f, r;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	f = asset_file_open(file);
	if ( NULL == f )
		goto out_free;

	r = asset_file_open("data/rotor.db");
	if ( NULL == f )
		goto out_free_file;

	c->fuselage = asset_file_get(f, "fuselage_green.g");
	if ( NULL == c->fuselage )
		goto out_free_rotor;

	c->glass = asset_file_get(f, "fuselage_black.g");
	if ( NULL == c->glass)
		goto out_free_fuselage;

	c->rotor = asset_file_get(r, "rotor.g");
	if ( NULL == c->rotor )
		goto out_free_glass;

	c->asset = f;
	c->rotor_asset = r;
	c->x = x;
	c->y = y;
	c->heading = h;
	INIT_LIST_HEAD(&c->missiles);
	chopper_think(c);

	/* success */
	goto out;

out_free_glass:
	asset_put(c->glass);
out_free_fuselage:
	asset_put(c->fuselage);
out_free_rotor:
	asset_file_close(r);
out_free_file:
	asset_file_close(f);
out_free:
	free(c);
	c = NULL;
out:
	return c;
}

chopper_t chopper_comanche(float x, float y, float h)
{
	return get_chopper("data/comanche.db", x, y, h);
}

void chopper_render(chopper_t chopper, renderer_t r, float lerp, light_t l)
{
	float heading;

	heading = chopper->oldheading - (chopper->avelocity * lerp);

	glPushMatrix();
	renderer_rotate(r, heading * (180.0 / M_PI), 0, 1, 0);
	renderer_rotate(r, chopper->velocity * 5.0, 1, 0, 0);
	renderer_rotate(r, 3.0 * chopper->velocity * (chopper->avelocity * M_PI * 2.0), 0, 0, 1);

	glColor4f(0.15, 0.2, 0.15, 1.0);

	if ( chopper->oldheading != chopper->heading ||
			chopper->oldvelocity != chopper->velocity )
		asset_file_dirty_shadows(chopper->asset);
	asset_file_render_begin(chopper->asset, r, l);
	asset_render(chopper->fuselage, r, l);

	glColor4f(0.1, 0.1, 0.1, 1.0);
	glFlush();
	asset_render(chopper->glass, r, l);
	asset_file_render_end(chopper->asset);

	glColor4f(0.15, 0.15, 0.15, 1.0);
	renderer_rotate(r, lerp * (72.0), 0, 1, 0);
	glFlush();
	if ( chopper->oldheading != chopper->heading ||
			chopper->oldvelocity != chopper->velocity ||
			chopper->oldlerp != lerp )
		asset_file_dirty_shadows(chopper->rotor_asset);
	asset_file_render_begin(chopper->rotor_asset, r, l);
	asset_render(chopper->rotor, r, l);
	asset_file_render_end(chopper->rotor_asset);

	glPopMatrix();

	chopper->oldlerp = lerp;
}

void chopper_free(chopper_t chopper)
{
	if ( chopper ) {
		asset_put(chopper->fuselage);
		asset_put(chopper->glass);
		asset_put(chopper->rotor);
		asset_file_close(chopper->rotor_asset);
		asset_file_close(chopper->asset);
		free(chopper);
	}
}


void chopper_render_missiles(chopper_t c, renderer_t r,
				float lerp, light_t l)
{
	struct missile *m, *tmp;

	asset_file_render_begin(c->asset, r, l);
	list_for_each_entry_safe(m, tmp, &c->missiles, m_list) {
		float x, y;
		glPushMatrix();
		glColor4f(0.0, 0.0, 1.0, 1.0);
		glFlush();

		x = m->m_oldorigin[0] + (m->m_velocity * sin(m->m_heading)) * lerp;
		y = m->m_oldorigin[2] + (m->m_velocity * cos (m->m_heading)) * lerp;
		glTranslatef(x, m->m_origin[1], y);
		renderer_rotate(r, m->m_heading * (180.0 / M_PI), 0, 1, 0);
		asset_render(m->m_mesh, r, l);
		glPopMatrix();
	}
	asset_file_render_end(c->asset);
}

static void missile_think(struct missile *m)
{
	m->m_lifetime--;
	if ( !m->m_lifetime ) {
		list_del(&m->m_list);
		asset_put(m->m_mesh);
		free(m);
		return;
	}

	memcpy(m->m_oldorigin, m->m_origin, sizeof(m->m_oldorigin));
	m->m_origin[0] += m->m_velocity * sin(m->m_heading);
	m->m_origin[2] += m->m_velocity * cos(m->m_heading);
}

void chopper_fire(chopper_t c, unsigned int time)
{
	struct missile *m;

	if ( c->last_fire + 5 > time ) {
		return;
	}

	m = calloc(1, sizeof(*m));
	if ( NULL == m )
		return;

	m->m_mesh = asset_file_get(c->asset, "AGR_71_Hydra.g");
	if ( NULL == m->m_mesh )
		goto err_free;

	m->m_heading = c->heading;
	m->m_lifetime = 20;
	m->m_velocity = 10;
	m->m_origin[0] = -(c->x + sin(m->m_heading) * m->m_velocity);
	m->m_origin[1] = 00.0f;
	m->m_origin[2] = -(c->y + cos(m->m_heading) * m->m_velocity);

	list_add_tail(&m->m_list, &c->missiles);
	missile_think(m);
	c->last_fire = time;

	return;

err_free:
	free(m);
}

static void missiles_think(chopper_t c)
{
	struct missile *m, *tmp;
	list_for_each_entry_safe(m, tmp, &c->missiles, m_list) {
		missile_think(m);
	}
}

void chopper_think(chopper_t chopper)
{
	int tctrl = 0;
	int rctrl = 0;

	/* first sum all inputs to total throttle and cyclical control */
	if ( chopper->input & (1 << CHOPPER_THROTTLE) )
		tctrl += 1;
	if ( chopper->input & (1 << CHOPPER_BRAKE) )
		tctrl -= 1;
	if ( chopper->input & (1 << CHOPPER_LEFT) )
		rctrl += 1;
	if ( chopper->input & (1 << CHOPPER_RIGHT) )
		rctrl -= 1;

	switch(tctrl) {
	case 0:
		/* no throttle control, coast down to stationary */
		if ( chopper->throttle_time > 0 )
			chopper->throttle_time--;
		else if ( chopper->throttle_time < 0 )
			chopper->throttle_time++;
		break;
	case 1:
		/* add throttle time for acceleration */
		if ( chopper->throttle_time < VELOCITY_INCREMENTS ) {
			chopper->throttle_time++;
		}
		break;
	case -1:
		/* add brake time for deceleration */
		if ( chopper->throttle_time > -VELOCITY_INCREMENTS ) {
			chopper->throttle_time--;
		}
		break;
	default:
		abort();
		break;
	}

	/* calculate velocity */
	chopper->oldvelocity = chopper->velocity;
	chopper->velocity = chopper->throttle_time * VELOCITY_UNIT;

	switch(rctrl) {
	case -1:
		chopper->avelocity = ANGLE_INCREMENT;
		break;
	case 1:
		chopper->avelocity = -ANGLE_INCREMENT;
		break;
	case 0:
		chopper->avelocity = 0;
		break;
	default:
		abort();
	}

	chopper->oldx = chopper->x;
	chopper->oldy = chopper->y;
	chopper->oldheading = chopper->heading;

	chopper->x -= chopper->velocity * sin(chopper->heading);
	chopper->y -= chopper->velocity * cos(chopper->heading);
	chopper->heading -= chopper->avelocity;

	missiles_think(chopper);
}

void chopper_control(chopper_t chopper, unsigned int ctrl, int down)
{
	switch(ctrl) {
	case CHOPPER_THROTTLE:
	case CHOPPER_BRAKE:
	case CHOPPER_LEFT:
	case CHOPPER_RIGHT:
		if ( down ) {
			chopper->input |= (1 << ctrl);
		}else{
			chopper->input &= ~(1 << ctrl);
		}
		break;
	default:
		abort();
		break;
	}
}
