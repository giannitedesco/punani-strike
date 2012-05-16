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
#include <punani/cvar.h>
#include <punani/map.h>
#include <punani/entity.h>
#include <punani/missile.h>
#include <math.h>
#include "ent-internal.h"
#include "list.h"

#define VELOCITY_INCREMENTS	8
#define VELOCITY_UNIT		1

static unsigned int rotation_steps = 5;
static float rotation_max = 0.4f;

#define ALTITUDE_INCREMENTS	7
#define ALTITUDE_UNIT		1

#define ANGLE_INCREMENT		((M_PI * 2.0) / 18)

struct _chopper {
	struct _entity ent;

	asset_file_t asset;
	asset_file_t rotor_asset;
	asset_t fuselage;
	asset_t rotor;

	float oldlerp;
	unsigned int input;

	unsigned int last_fire;

	int f_throttle_time; /* in frames */
	int s_throttle_time; /* in frames */
	int rot_throttle_time;  /* in frames */
	int alt_throttle_time; /* in frames */

	float f_velocity; /* in meters per frame */
	float s_velocity; /* in meters per frame */
	float rot_velocity;
	float alt_velocity; /* in meters per frame */

	float heading; /* in radians */

	float oldf_velocity;
	float oldfselocity;
	float oldheading;

	cvar_ns_t cvars;
};

void chopper_get_pos(chopper_t c, float lerp, vec3_t out)
{
	out[0] = c->ent.e_oldorigin[0] + c->ent.e_move[0] * lerp;
	out[1] = c->ent.e_oldorigin[1] + c->ent.e_move[1] * lerp;
	out[2] = c->ent.e_oldorigin[2] + c->ent.e_move[2] * lerp;
}

static void linear_velocity_think(const int ctrl, int *vel_throttle_time, float *vel_velocity, const int vel_increments, const float vel_unit)
{
	switch(ctrl) {
	case 0:
		/* no throttle control, coast down to stationary */
		if ( *vel_throttle_time > 0 )
			(*vel_throttle_time)--;
		else if ( *vel_throttle_time < 0 )
			(*vel_throttle_time)++;
		break;
	case 1:
		/* add throttle time for acceleration */
		if ( *vel_throttle_time < vel_increments ) {
			(*vel_throttle_time)++;
		}
		break;
	case -1:
		/* add brake time for deceleration */
		if ( *vel_throttle_time > -vel_increments ) {
			(*vel_throttle_time)--;
		}
		break;
	default:
		abort();
		break;
	}
	*vel_velocity = *vel_throttle_time * vel_unit;
}

static void quad_velocity_think(const int ctrl, int *vel_throttle_time, float *vel_velocity, const int vel_steps, const float vel_max)
{
	/* based around  quad(x) = 1 + (-1 * ((x-1)*(x-1))), giving a smooth increase from 0..1 over an 0..1 input range. */
	switch(ctrl) {
	case 0:
		/* no throttle control, coast down to stationary */
		if ( *vel_throttle_time > 0 )
			(*vel_throttle_time)--;
		else if ( *vel_throttle_time < 0 )
			(*vel_throttle_time)++;
		break;
	case 1:
		/* add throttle time for acceleration */
		if ( *vel_throttle_time < vel_steps ) {
			(*vel_throttle_time)++;
		}
		break;
	case -1:
		/* add brake time for deceleration */
		if ( *vel_throttle_time > -vel_steps ) {
			(*vel_throttle_time)--;
		}
		break;
	default:
		abort();
		break;
	}

	float quad;
	float x;
	float ax;

	/* ranges from -1 .. 0 .. +1 */
	x = (*vel_throttle_time) / (float)vel_steps;
	ax = fabs(x);
	quad = 1 + (-1 * ((ax-1)*(ax-1)));
	*vel_velocity = quad * x * vel_max;
}


static void e_dtor(struct _entity *e)
{
	struct _chopper *c = (struct _chopper *)e;
	asset_put(c->fuselage);
	asset_put(c->rotor);
	asset_file_close(c->rotor_asset);
	asset_file_close(c->asset);
	cvar_ns_save(c->cvars);
	cvar_ns_free(c->cvars);
	free(c);
}

static void e_think(struct _entity *e)
{
	struct _chopper *c = (struct _chopper *)e;
	int tctrl = 0;
	int rctrl = 0;
	int sctrl = 0;
	/* OMG initilisation at declaration */
	int actrl = 0;

	/* first sum all inputs to total throttle and cyclical control */
	if ( c->input & (1 << CHOPPER_THROTTLE) )
		tctrl += 1;
	if ( c->input & (1 << CHOPPER_BRAKE) )
		tctrl -= 1;
	if ( c->input & (1 << CHOPPER_ROTATE_LEFT) )
		rctrl += 1;
	if ( c->input & (1 << CHOPPER_ROTATE_RIGHT) )
		rctrl -= 1;
	if ( c->input & (1 << CHOPPER_STRAFE_LEFT) )
		sctrl -= 1;
	if ( c->input & (1 << CHOPPER_STRAFE_RIGHT) )
		sctrl += 1;
	if ( c->input & (1 << CHOPPER_ALTITUDE_INC) )
		actrl += 1;
	if ( c->input & (1 << CHOPPER_ALTITUDE_DEC) )
		actrl -= 1;

	/* calculate velocity */
	c->oldf_velocity = c->f_velocity;

	linear_velocity_think(tctrl, &c->f_throttle_time, &c->f_velocity, VELOCITY_INCREMENTS, VELOCITY_UNIT);
	quad_velocity_think(rctrl, &c->rot_throttle_time, &c->rot_velocity, rotation_steps, rotation_max);
	linear_velocity_think(actrl, &c->alt_throttle_time, &c->alt_velocity, ALTITUDE_INCREMENTS, ALTITUDE_UNIT);
	linear_velocity_think(sctrl, &c->s_throttle_time, &c->s_velocity, ALTITUDE_INCREMENTS, ALTITUDE_UNIT);

	v_copy(c->ent.e_oldorigin, c->ent.e_origin);
	c->oldheading = c->heading;

	c->ent.e_move[0] = (c->f_velocity * sin(c->heading)) + (c->s_velocity * sin(c->heading - M_PI_2));
	c->ent.e_move[1] = c->alt_velocity;
	c->ent.e_move[2] = (c->f_velocity * cos(c->heading)) + (c->s_velocity * cos(c->heading - M_PI_2));

	v_add(c->ent.e_origin, c->ent.e_move);
	c->heading += c->rot_velocity;
}

static void e_render(struct _entity *e, renderer_t r, float lerp, light_t l)
{
	struct _chopper *c = (struct _chopper *)e;
	float heading;
	vec3_t pos;

	heading = c->oldheading + (c->rot_velocity * lerp);
	chopper_get_pos(c, lerp, pos);

	renderer_rotate(r, heading * (180.0 / M_PI), 0, 1, 0);
	renderer_rotate(r, c->f_velocity * 5.0, 1, 0, 0);
	renderer_rotate(r, 3.0 * c->f_velocity * (-c->rot_velocity * M_PI * 2.0), 0, 0, 1);
	renderer_rotate(r, c->s_velocity * 3.0, 0, 0, 1);

	asset_file_dirty_shadows(c->asset);
	asset_file_render_begin(c->asset, r, l);
	asset_render(c->fuselage, r, l);

	renderer_rotate(r, lerp * (72.0), 0, 1, 0);

	asset_file_dirty_shadows(c->rotor_asset);
	asset_file_render_begin(c->rotor_asset, r, l);
	asset_render(c->rotor, r, l);
	asset_file_render_end(c->rotor_asset);

	c->oldlerp = lerp;
}

static float e_radius(struct _entity *e)
{
	struct _chopper *c = (struct _chopper *)e;
	return asset_radius(c->fuselage);
}

static const struct entity_ops e_ops = {
	.e_flags = ENT_HELI,
	.e_render = e_render,
	.e_think = e_think,
	.e_radius = e_radius,
	.e_dtor = e_dtor,
};

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
	c->heading = heading;

	c->cvars = cvar_ns_new("chopper");

	cvar_register_float(c->cvars, "height",
				CVAR_FLAG_SAVE_NOTDEFAULT,
				&c->ent.e_origin[1]);
	cvar_register_float(c->cvars, "rot_max",
				CVAR_FLAG_SAVE_NOTDEFAULT,
				&rotation_max);
	cvar_register_uint(c->cvars, "rot_steps",
				CVAR_FLAG_SAVE_NOTDEFAULT,
				&rotation_steps);

	cvar_ns_load(c->cvars);

	entity_spawn(&c->ent, &e_ops, pos, NULL);
	entity_link(&c->ent);

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

void chopper_free(chopper_t c)
{
	entity_unlink(&c->ent);
}

void chopper_fire(chopper_t c, unsigned int time)
{
	float pitch;

	if ( c->last_fire + 5 > time ) {
		return;
	}

	pitch = (M_PI / 2.0) + (c->f_velocity / (VELOCITY_INCREMENTS * VELOCITY_UNIT)) * (M_PI / 2.0);
	pitch += M_PI / 8.0;
	if ( pitch < M_PI / 2.0 )
		pitch = M_PI / 2.0;

	missile_spawn(c->ent.e_origin, c->heading, pitch);
	c->last_fire = time;
}

void chopper_control(chopper_t c, unsigned int ctrl, int down)
{
	switch(ctrl) {
	case CHOPPER_THROTTLE:
	case CHOPPER_BRAKE:
	case CHOPPER_ROTATE_LEFT:
	case CHOPPER_ROTATE_RIGHT:
	case CHOPPER_STRAFE_LEFT:
	case CHOPPER_STRAFE_RIGHT:
	case CHOPPER_ALTITUDE_INC:
	case CHOPPER_ALTITUDE_DEC:
		if ( down ) {
			c->input |= (1 << ctrl);
		}else{
			c->input &= ~(1 << ctrl);
		}
		break;
	default:
		abort();
		break;
	}
}

void chopper_control_release_all(chopper_t c)
{
	c->input = 0;
}

