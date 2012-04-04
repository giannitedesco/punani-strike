/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/chopper.h>
#include <math.h>
#include "list.h"

#define CHOPPER_NUM_ANGLES	24
#define CHOPPER_NUM_SPRITES	13
#define CHOPPER_NUM_PITCHES	4
#define CHOPPER_BANK_LEFT	0
#define CHOPPER_BANK_RIGHT	1
#define CHOPPER_NUM_BANKS	2

#define VELOCITY_INCREMENTS	7
#define VELOCITY_UNIT		5 /* pixels per frame */

#define NUM_ROTORS		5

#define ANGLE_INCREMENT		((M_PI * 2.0) / CHOPPER_NUM_ANGLES)

struct chopper_angle {
	texture_t pitch[CHOPPER_NUM_PITCHES];
	texture_t bank[CHOPPER_NUM_BANKS];
};

struct rotor_gfx {
	unsigned int refcnt;
	texture_t rotor[NUM_ROTORS];
};

struct chopper_gfx {
	char *name;
	unsigned int num_pitches;
	unsigned int refcnt;
	struct chopper_angle angle[CHOPPER_NUM_ANGLES];
	struct rotor_gfx *rotor;
	struct list_head list;
};

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
	unsigned int pitch;
	unsigned int input;
	unsigned int rotor;
	int bank;

	int throttle_time; /* in frames */
	float velocity; /* in pixels per frame */
	float heading; /* in radians */

	struct chopper_gfx *gfx;
};

static LIST_HEAD(gfx_list);

static unsigned int angle_idx2file(unsigned int angle, unsigned int *flip)
{
	if ( angle < CHOPPER_NUM_SPRITES ) {
		*flip = 0;
		return angle;
	}
	*flip = 1;
	return CHOPPER_NUM_ANGLES - (angle - 0);
}

static int load_pitch(renderer_t r, struct chopper_gfx *gfx, unsigned int angle,
			unsigned int pitch)
{
	char buf[512];
	unsigned int flip;

	snprintf(buf, sizeof(buf), "data/chopper/%s_%d_%d.png",
		gfx->name, angle_idx2file(angle, &flip), pitch);

	gfx->angle[angle].pitch[pitch] = png_get_by_name(r, buf, flip);
	if ( NULL == gfx->angle[angle].pitch[pitch] )
		return 0;

	return 1;
}

static int load_bank(renderer_t r, struct chopper_gfx *gfx, unsigned int angle,
			unsigned int bank)
{
	char buf[512];
	unsigned int a, flip;

	a = angle_idx2file(angle, &flip);

	snprintf(buf, sizeof(buf), "data/chopper/%s_%d_%s.png",
		gfx->name, a, (bank ^ flip) ? "bl" : "br");

	gfx->angle[angle].bank[bank] = png_get_by_name(r, buf, flip);
	if ( NULL == gfx->angle[angle].bank[bank] )
		return 0;

	return 1;
}

static texture_t current_tex(chopper_t chopper)
{
	texture_t ret;

	switch(chopper->bank) {
	case CHOPPER_BANK_LEFT:
	case CHOPPER_BANK_RIGHT:
		ret = chopper->gfx->angle[chopper->angle].bank[chopper->bank];
		break;
	default:
		ret = chopper->gfx->angle[chopper->angle].pitch[chopper->pitch];
		break;
	}

	return ret;
}

static void put_rotor_gfx(struct rotor_gfx *r)
{
	unsigned int i;

	r->refcnt--;
	if ( r->refcnt )
		return;

	for(i = 0; i < NUM_ROTORS; i++) {
		texture_put(r->rotor[i]);
	}
}

static struct rotor_gfx *get_rotor_gfx(renderer_t r)
{
	static struct rotor_gfx gfx;
	unsigned int i;

	if ( gfx.refcnt ) {
		gfx.refcnt++;
		return &gfx;
	}

	gfx.refcnt = 1;

	for(i = 0; i < NUM_ROTORS; i++) {
		char buf[512];
		snprintf(buf, sizeof(buf), "data/chopper/nrotor%d.png", i);
		gfx.rotor[i] = png_get_by_name(r, buf, 0);
		if ( NULL == gfx.rotor[i] ) {
			put_rotor_gfx(&gfx);
			return NULL;
		}
	}

	return &gfx;
}

void chopper_get_size(chopper_t chopper, unsigned int *x, unsigned int *y)
{
	texture_t tex;

	tex = current_tex(chopper);

	if ( x )
		*x = texture_width(tex);
	if ( y )
		*y = texture_height(tex);
}

void chopper_get_pos(chopper_t chopper, unsigned int *x, unsigned int *y)
{
	if ( x )
		*x = chopper->x;
	if ( y )
		*y = chopper->y;
}

static int load_angle(renderer_t r, struct chopper_gfx *gfx, unsigned int angle)
{
	unsigned int i;

	for(i = 0; i < gfx->num_pitches; i++) {
		if ( !load_pitch(r, gfx, angle, i) )
			return 0;
	}

	for(i = 0; i < CHOPPER_NUM_BANKS; i++) {
		if ( !load_bank(r, gfx, angle, i) )
			return 0;
	}

	return 1;
}

static struct chopper_gfx *gfx_get(renderer_t r, const char *name,
					unsigned int num_pitches)
{
	struct chopper_gfx *gfx;
	unsigned int i;

	list_for_each_entry(gfx, &gfx_list, list) {
		if ( !strcmp(name, gfx->name) ) {
			gfx->refcnt++;
			return gfx;
		}
	}

	gfx = calloc(1, sizeof(*gfx));
	if ( NULL == gfx )
		goto out;

	gfx->num_pitches = num_pitches;
	gfx->name = strdup(name);
	if ( NULL == gfx->name )
		goto out_free;

	for(i = 0; i < CHOPPER_NUM_ANGLES; i++) {
		if ( !load_angle(r, gfx, i) )
			goto out_free_all;
	}

	gfx->rotor = get_rotor_gfx(r);
	if ( NULL == gfx->rotor )
		goto out_free_all;

	gfx->refcnt = 1;
	list_add_tail(&gfx->list, &gfx_list);
	goto out;

out_free_all:
	for(i = 0; i < CHOPPER_NUM_ANGLES; i++) {
		unsigned int j;
		for(j = 0; j < CHOPPER_NUM_PITCHES; j++)
			texture_put(gfx->angle[i].pitch[j]);
		for(j = 0; j < CHOPPER_NUM_BANKS; j++)
			texture_put(gfx->angle[i].bank[j]);
	}

	free(gfx->name);
out_free:
	free(gfx);
	gfx = NULL;
out:
	return gfx;
}

static void gfx_free(struct chopper_gfx *gfx)
{
	unsigned int i;

	for(i = 0; i < CHOPPER_NUM_ANGLES; i++) {
		unsigned int j;
		for(j = 0; j < CHOPPER_NUM_PITCHES; j++)
			texture_put(gfx->angle[i].pitch[j]);
		for(j = 0; j < CHOPPER_NUM_BANKS; j++)
			texture_put(gfx->angle[i].bank[j]);
	}

	put_rotor_gfx(gfx->rotor);
	free(gfx->name);
	free(gfx);
}

static void gfx_put(struct chopper_gfx *gfx)
{
	gfx->refcnt--;
	if ( !gfx->refcnt )
		gfx_free(gfx);
}

static chopper_t get_chopper(renderer_t r, const char *name,
				unsigned int num_pitches,
				unsigned int x, unsigned int y, float h)
{
	struct _chopper *c = NULL;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	c->gfx = gfx_get(r, name, num_pitches);
	if ( NULL == c->gfx )
		goto out_free;

	c->bank = -1;
	c->angle = 6;
	c->pitch = 0;
	c->x = x;
	c->y = y;
	c->heading = h;
	chopper_think(c);

	/* success */
	goto out;

out_free:
	free(c);
	c = NULL;
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
	texture_t tex;
	prect_t dst;

	tex = current_tex(chopper);

	dst.x = chopper->x;
	dst.y = chopper->y;
	dst.h = texture_height(tex);
	dst.w = texture_width(tex);

	renderer_blit(r, tex, NULL, &dst);
	renderer_blit(r, chopper->gfx->rotor->rotor[chopper->rotor],
			NULL, &dst);
}

void chopper_free(chopper_t chopper)
{
	if ( chopper ) {
		gfx_put(chopper->gfx);
		free(chopper);
	}
}

static unsigned int heading2angle(float heading, unsigned long div)
{
	float rads_per_div = (M_PI * 2.0) / div;
	float ret;
	heading = remainder(heading, M_PI * 2.0);
	if ( heading < 0 )
		heading = (M_PI * 2.0) - fabs(heading);
	ret = div - (heading / rads_per_div);
	return lround(ret) % div;
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

	/* figure out which pitch sprite to render */
	switch(chopper->throttle_time) {
	case 0:
		chopper->pitch = 1;
		break;
	case 1:
		chopper->pitch = 2;
		break;
	case 2 ... 100:
		chopper->pitch = 3;
		break;
	case -100 ... -1:
		chopper->pitch = 0;
		break;
	}

	if ( chopper->pitch >= chopper->gfx->num_pitches )
		chopper->pitch = chopper->gfx->num_pitches - 1;

	switch(rctrl) {
	case -1:
//		chopper->bank = CHOPPER_BANK_LEFT;
		chopper->heading -= ANGLE_INCREMENT;
		break;
	case 1:
//		chopper->bank = CHOPPER_BANK_RIGHT;
		chopper->heading += ANGLE_INCREMENT;
		break;
	case 0:
		chopper->bank = -1;
		break;
	default:
		abort();
	}

	chopper->oldx = chopper->x;
	chopper->oldy = chopper->y;
	chopper->angle = abs(heading2angle(chopper->heading,
						CHOPPER_NUM_ANGLES));
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
