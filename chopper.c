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

struct _chopper {
	asset_file_t asset;
	asset_t fuselage;
	asset_t glass;
	asset_t rotor;

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

	int throttle_time; /* in frames */
	float velocity; /* in pixels per frame */
	float heading; /* in radians */
	float avelocity;
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
	asset_file_t f;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	f = asset_file_open(file);
	if ( NULL == f )
		goto out_free;

	c->fuselage = asset_file_get(f, "fuselage_green.g");
	if ( NULL == c->fuselage )
		goto out_free_file;

	c->glass = asset_file_get(f, "fuselage_black.g");
	if ( NULL == c->glass)
		goto out_free_fuselage;

	c->rotor = asset_file_get(f, "rotor.g");
	if ( NULL == c->rotor )
		goto out_free_glass;

	c->asset = f;
	c->x = x;
	c->y = y;
	c->heading = h;
	chopper_think(c);

	/* success */
	goto out;

out_free_glass:
	asset_put(c->glass);
out_free_fuselage:
	asset_put(c->fuselage);
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
			chopper->oldlerp != lerp )
		asset_file_dirty_shadows(chopper->asset);
	asset_file_render_begin(chopper->asset, r, l);
	asset_render(chopper->fuselage, r, l);

	glColor4f(0.1, 0.1, 0.1, 1.0);
	glFlush();
	asset_render(chopper->glass, r, l);

	/* FIXME: rotor shadow needs re-calculating every time it rotates */
	glColor4f(0.15, 0.15, 0.15, 1.0);
	renderer_rotate(r, lerp * (72.0), 0, 1, 0);
	glFlush();
	if ( NULL == l )
		asset_render(chopper->rotor, r, l);
	asset_file_render_end(chopper->asset);

	glPopMatrix();

	chopper->oldlerp = lerp;
}

void chopper_free(chopper_t chopper)
{
	if ( chopper ) {
		asset_put(chopper->fuselage);
		asset_put(chopper->glass);
		asset_put(chopper->rotor);
		asset_file_close(chopper->asset);
		free(chopper);
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
