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
#include <GL/gl.h>
#include <math.h>
#include "list.h"

#define VELOCITY_INCREMENTS	7
#define VELOCITY_UNIT		1

#define ANGLE_INCREMENT		((M_PI * 2.0) / 18)

struct _chopper {
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
	unsigned int input;
	int bank;

	int throttle_time; /* in frames */
	float velocity; /* in pixels per frame */
	float heading; /* in radians */
	float avelocity;
	float oldheading;
};

static asset_file_t chopper_gfx;
static unsigned int chopper_refcnt;

static asset_t gfx_get(const char *name)
{
	asset_t ret;
	if ( !chopper_refcnt ) {
		assert(chopper_gfx == NULL);
		chopper_gfx = asset_file_open("data/choppers.db");
		if ( NULL == chopper_gfx )
			return NULL;
	}

	ret = asset_file_get(chopper_gfx, name);
	if ( ret )
		chopper_refcnt++;
	return ret;
}

static void gfx_put(asset_t asset)
{
	asset_put(asset);
	chopper_refcnt--;
	if ( !chopper_refcnt ) {
		asset_file_close(chopper_gfx);
		chopper_gfx = NULL;
	}
}

void chopper_get_pos(chopper_t chopper, float *x, float *y)
{
	if ( x )
		*x = chopper->x;
	if ( y )
		*y = chopper->y;
}

static chopper_t get_chopper(const char *fuselage,
				const char *glass,
				const char *rotor,
				float x, float y, float h)
{
	struct _chopper *c = NULL;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	c->fuselage = gfx_get(fuselage);
	if ( NULL == c->fuselage )
		goto out_free;

	c->glass = gfx_get(glass);
	if ( NULL == c->glass)
		goto out_free_fuselage;

	c->rotor = gfx_get(rotor);
	if ( NULL == c->rotor )
		goto out_free_glass;

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
out_free:
	free(c);
	c = NULL;
out:
	return c;
}

chopper_t chopper_comanche(float x, float y, float h)
{
	return get_chopper("chopper/fuselage_green.g",
				"chopper/fuselage_black.g",
				"chopper/rotor.g",
				x, y, h);
}

void chopper_render(chopper_t chopper, renderer_t r, float lerp, light_t l)
{
	chopper->x = chopper->oldx -
		(chopper->velocity * lerp) * sin(chopper->heading);
	chopper->y = chopper->oldy -
		(chopper->velocity * lerp) * cos(chopper->heading);
	chopper->heading = chopper->oldheading -
		(chopper->avelocity * lerp);

	asset_file_render_begin(chopper_gfx);
	glPushMatrix();
	renderer_translate(r, -chopper->x, 20.0, -chopper->y);
	renderer_rotate(r, chopper->heading * (180.0 / M_PI), 0, 1, 0);
	renderer_rotate(r, chopper->velocity * 2.5, 1, 0, 0);
	renderer_rotate(r, 3.0 * chopper->velocity * (chopper->avelocity * M_PI * 2.0), 0, 0, 1);

	glColor4f(0.15, 0.2, 0.15, 1.0);
	asset_render(chopper->fuselage, r, l);

	glColor4f(0.1, 0.1, 0.1, 1.0);
	glFlush();
	asset_render(chopper->glass, r, l);

	/* rendering the rotor shadows seems to go a bit mental but
	 * maybe just needs optimising. in either case probably best
	 * to either do cinematic style "slow backwards" type of rotation
	 * or just render a blurry disc of shadow
	*/
	if ( NULL == l ) {
		glColor4f(0.15, 0.15, 0.15, 1.0);
		renderer_rotate(r, lerp * (72.0), 0, 1, 0);
		glFlush();
		asset_render(chopper->rotor, r, l);
	}

	glPopMatrix();
	asset_file_render_end(chopper_gfx);
}

void chopper_free(chopper_t chopper)
{
	if ( chopper ) {
		gfx_put(chopper->fuselage);
		gfx_put(chopper->glass);
		gfx_put(chopper->rotor);
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
