/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/chopper.h>
#include <math.h>
#include "list.h"

#define VELOCITY_INCREMENTS	7
#define VELOCITY_UNIT		5 /* pixels per frame */

#define NUM_ROTORS		5

#define ANGLE_INCREMENT		((M_PI * 2.0) / 36)

struct _chopper {
	/* these ones are calculated based on "physics"
	 * vs. control surfaces and are the final answer
	 * as to what we will render in the next pass
	*/
	unsigned int x;
	unsigned int y;
	unsigned int oldx;
	unsigned int oldy;
	unsigned int angle;
	unsigned int input;
	unsigned int rotor;
	int bank;

	int throttle_time; /* in frames */
	float velocity; /* in pixels per frame */
	float heading; /* in radians */
};

void chopper_get_size(chopper_t chopper, unsigned int *x, unsigned int *y)
{
	if ( x )
		*x = 64;
	if ( y )
		*y = 64;
}

void chopper_get_pos(chopper_t chopper, unsigned int *x, unsigned int *y)
{
	if ( x )
		*x = chopper->x;
	if ( y )
		*y = chopper->y;
}

static chopper_t get_chopper(renderer_t r, const char *name,
				unsigned int num_pitches,
				unsigned int x, unsigned int y, float h)
{
	struct _chopper *c = NULL;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	c->x = x;
	c->y = y;
	c->heading = h;
	chopper_think(c);

	/* success */
	goto out;

out:
	return c;
}

chopper_t chopper_apache(renderer_t r, unsigned int x, unsigned int y, float h)
{
	return get_chopper(r, "apache", 4, x, y, h);
}

chopper_t chopper_comanche(renderer_t r, unsigned int x, unsigned int y,
				float h)
{
	return get_chopper(r, "comanche", 3, x, y, h);
}

void chopper_pre_render(chopper_t chopper, float lerp)
{
	chopper->x = chopper->oldx - (chopper->velocity * lerp) * sin(chopper->heading);
	chopper->y = chopper->oldy - (chopper->velocity * lerp) * cos(chopper->heading);
}

void chopper_render(chopper_t chopper, renderer_t r, float lerp)
{
}

void chopper_free(chopper_t chopper)
{
	if ( chopper ) {
		free(chopper);
	}
}

void chopper_think(chopper_t chopper)
{
	int tctrl = 0;
	int rctrl = 0;

	chopper->rotor = (chopper->rotor + 2) % NUM_ROTORS;

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
		chopper->heading -= ANGLE_INCREMENT;
		break;
	case 1:
		chopper->heading += ANGLE_INCREMENT;
		break;
	case 0:
		break;
	default:
		abort();
	}

	chopper->oldx = chopper->x;
	chopper->oldy = chopper->y;
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
