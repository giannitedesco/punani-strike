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
#include <punani/particles.h>
#include <punani/punani_gl.h>
#include <punani/cvar.h>
#include <math.h>
#include "list.h"

#define VELOCITY_INCREMENTS	7
#define VELOCITY_UNIT		1

#define ANGLE_INCREMENT		((M_PI * 2.0) / 18)

struct missile {
	struct list_head m_list;
	asset_t m_mesh;
	particles_t m_trail;
	vec3_t m_origin;
	vec3_t m_oldorigin;
	vec3_t m_move;
	vec3_t m_oldlerp;
	float m_velocity;
	float m_heading;
	unsigned int m_lifetime;
};

struct _chopper {
	asset_file_t asset;
	asset_file_t rotor_asset;
	asset_t fuselage;
	asset_t rotor;

	struct list_head missiles;

	/* these ones are calculated based on "physics"
	 * vs. control surfaces and are the final answer
	 * as to what we will render in the next pass
	*/
	vec3_t origin;
	vec3_t oldorigin;
	vec3_t move;

	float oldlerp;
	unsigned int input;

	unsigned int last_fire;
	float missile_speed;

	int f_throttle_time; /* in frames */
	int s_throttle_time; /* in frames */

	float fvelocity; /* in pixels per frame */
	float svelocity; /* in pixels per frame */
	float avelocity;

	float heading; /* in radians */

	float oldfvelocity;
	float oldfselocity;
	float oldheading;
};

void chopper_get_pos(chopper_t chopper, float lerp, vec3_t out)
{
	out[0] = chopper->oldorigin[0] + chopper->move[0] * lerp;
	out[1] = chopper->oldorigin[1] + chopper->move[1] * lerp;
	out[2] = chopper->oldorigin[2] + chopper->move[2] * lerp;
}

static chopper_t get_chopper(const char *file, const vec3_t pos, float heading)
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

	c->fuselage = asset_file_get(f, "fuselage.g");
	if ( NULL == c->fuselage )
		goto out_free_rotor;

	c->rotor = asset_file_get(r, "rotor.g");
	if ( NULL == c->rotor )
		goto out_free_fuselage;

	c->asset = f;
	c->rotor_asset = r;
	v_copy(c->origin, pos);
	v_copy(c->oldorigin, pos);
	c->heading = heading;
	c->missile_speed = 12;
	INIT_LIST_HEAD(&c->missiles);
	chopper_think(c);

	cvar_register_float("chopper", "height", &c->origin[1]);
	cvar_register_float("chopper", "missile_speed", &c->missile_speed);

	/* success */
	goto out;

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

chopper_t chopper_comanche(const vec3_t pos, float h)
{
	return get_chopper("data/apache.db", pos, h);
}

void chopper_render(chopper_t chopper, renderer_t r, float lerp, light_t l)
{
	float heading;

	heading = chopper->oldheading + (chopper->avelocity * lerp);

	glPushMatrix();
	renderer_rotate(r, heading * (180.0 / M_PI), 0, 1, 0);
	renderer_rotate(r, chopper->fvelocity * 5.0, 1, 0, 0);
	renderer_rotate(r, 3.0 * chopper->fvelocity * (-chopper->avelocity * M_PI * 2.0), 0, 0, 1);
	renderer_rotate(r, chopper->svelocity * 3.0, 0, 0, 1);

	glColor4f(0.15, 0.2, 0.15, 1.0);

	asset_file_dirty_shadows(chopper->asset);
	asset_file_render_begin(chopper->asset, r, l);
	asset_render(chopper->fuselage, r, l);

	glColor4f(0.15, 0.15, 0.15, 1.0);
	renderer_rotate(r, lerp * (72.0), 0, 1, 0);
	glFlush();

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

	list_for_each_entry_safe(m, tmp, &c->missiles, m_list) {
		vec3_t pos;
		
		glPushMatrix();
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glFlush();

		pos[0] = m->m_oldorigin[0] + m->m_move[0] * lerp;
		pos[1] = m->m_oldorigin[1] + m->m_move[1] * lerp;
		pos[2] = m->m_oldorigin[2] + m->m_move[2] * lerp;
		glTranslatef(pos[0], pos[1], pos[2]);
		renderer_rotate(r, m->m_heading * (180.0 / M_PI), 0, 1, 0);
		asset_file_dirty_shadows(c->asset);
		asset_file_render_begin(c->asset, r, l);
		asset_render(m->m_mesh, r, l);
		asset_file_render_end(c->asset);
		glPopMatrix();
		
		particles_emit(m->m_trail, pos, m->m_oldlerp);

		v_copy(m->m_oldlerp, pos );
	}
}

static void missile_think(struct missile *m)
{
	m->m_lifetime--;
	if ( !m->m_lifetime ) {
		list_del(&m->m_list);
		asset_put(m->m_mesh);
		particles_free(m->m_trail);
		free(m);
		return;
	}

	v_copy(m->m_oldorigin, m->m_origin);
	v_add(m->m_origin, m->m_move);
//	printf("missile %f %f %f\n", m->m_origin[0], m->m_origin[1], m->m_origin[2]);
}

void chopper_fire(chopper_t c, renderer_t r, unsigned int time)
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

	m->m_trail = particles_new(r, 1024);
	if ( NULL == m->m_trail )
		goto err_free_mesh;

	m->m_heading = c->heading;
	m->m_lifetime = 100;
	m->m_velocity = c->missile_speed;
	m->m_origin[0] = c->origin[0];
	m->m_origin[1] = c->origin[1];
	m->m_origin[2] = c->origin[2];
	m->m_move[0] += m->m_velocity * sin(m->m_heading);
	m->m_move[1] = 0.0;
	m->m_move[2] += m->m_velocity * cos(m->m_heading);
	v_copy(m->m_oldlerp, m->m_origin);
	//printf("Missile away: %f %f %f\n", m->m_origin[0], m->m_origin[1], m->m_origin[2]);

	list_add_tail(&m->m_list, &c->missiles);
	c->last_fire = time;

	v_copy(m->m_oldorigin, m->m_origin);
	return;

err_free_mesh:
	asset_put(m->m_mesh);
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
	int sctrl = 0;

	/* first sum all inputs to total throttle and cyclical control */
	if ( chopper->input & (1 << CHOPPER_THROTTLE) )
		tctrl += 1;
	if ( chopper->input & (1 << CHOPPER_BRAKE) )
		tctrl -= 1;
	if ( chopper->input & (1 << CHOPPER_ROTATE_LEFT) )
		rctrl += 1;
	if ( chopper->input & (1 << CHOPPER_ROTATE_RIGHT) )
		rctrl -= 1;
	if ( chopper->input & (1 << CHOPPER_STRAFE_LEFT) )
		sctrl -= 1;
	if ( chopper->input & (1 << CHOPPER_STRAFE_RIGHT) )
		sctrl += 1;

	switch(tctrl) {
	case 0:
		/* no throttle control, coast down to stationary */
		if ( chopper->f_throttle_time > 0 )
			chopper->f_throttle_time--;
		else if ( chopper->f_throttle_time < 0 )
			chopper->f_throttle_time++;
		break;
	case 1:
		/* add throttle time for acceleration */
		if ( chopper->f_throttle_time < VELOCITY_INCREMENTS ) {
			chopper->f_throttle_time++;
		}
		break;
	case -1:
		/* add brake time for deceleration */
		if ( chopper->f_throttle_time > -VELOCITY_INCREMENTS ) {
			chopper->f_throttle_time--;
		}
		break;
	default:
		abort();
		break;
	}

	switch(sctrl) {
	case 0:
		/* no throttle control, coast down to stationary */
		if ( chopper->s_throttle_time > 0 )
			chopper->s_throttle_time--;
		else if ( chopper->s_throttle_time < 0 )
			chopper->s_throttle_time++;
		break;
	case 1:
		/* add throttle time for acceleration */
		if ( chopper->s_throttle_time < VELOCITY_INCREMENTS ) {
			chopper->s_throttle_time++;
		}
		break;
	case -1:
		/* add brake time for deceleration */
		if ( chopper->s_throttle_time > -VELOCITY_INCREMENTS ) {
			chopper->s_throttle_time--;
		}
		break;
	default:
		abort();
		break;
	}

	/* calculate velocity */
	chopper->oldfvelocity = chopper->fvelocity;
	chopper->fvelocity = chopper->f_throttle_time * VELOCITY_UNIT;
	chopper->svelocity = chopper->s_throttle_time * VELOCITY_UNIT;

	switch(rctrl) {
	case -1:
		chopper->avelocity = -ANGLE_INCREMENT;
		break;
	case 1:
		chopper->avelocity = ANGLE_INCREMENT;
		break;
	case 0:
		chopper->avelocity = 0;
		break;
	default:
		abort();
		break;
	}

	v_copy(chopper->oldorigin, chopper->origin);
	chopper->oldheading = chopper->heading;

	chopper->move[0] = (chopper->fvelocity * sin(chopper->heading)) + (chopper->svelocity * sin(chopper->heading - M_PI_2));
	chopper->move[1] = 0.0;
	chopper->move[2] = (chopper->fvelocity * cos(chopper->heading)) + (chopper->svelocity * cos(chopper->heading - M_PI_2));

	v_add(chopper->origin, chopper->move);
	chopper->heading += chopper->avelocity;

	missiles_think(chopper);
}


void chopper_control(chopper_t chopper, unsigned int ctrl, int down)
{
	switch(ctrl) {
	case CHOPPER_THROTTLE:
	case CHOPPER_BRAKE:
	case CHOPPER_ROTATE_LEFT:
	case CHOPPER_ROTATE_RIGHT:
	case CHOPPER_STRAFE_LEFT:
	case CHOPPER_STRAFE_RIGHT:
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
