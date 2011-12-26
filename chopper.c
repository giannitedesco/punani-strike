#include <punani/punani.h>
#include <punani/game.h>
#include <punani/tex.h>
#include <punani/chopper.h>
#include "list.h"

#define CHOPPER_NUM_ANGLES	12
#define CHOPPER_NUM_PITCHES	4
#define CHOPPER_NUM_BANKS	2

#define VELOCITY_INCREMENTS	4
#define VELOCITY_UNIT		10 /* pixels per frame */

struct chopper_angle {
	texture_t pitch[CHOPPER_NUM_PITCHES];
	texture_t bank[CHOPPER_NUM_BANKS];
};

struct chopper_gfx {
	char *name;
	unsigned int refcnt;
	struct chopper_angle angle[CHOPPER_NUM_ANGLES];
	struct list_head list;
};

struct _chopper {
	/* these ones are calculated based on "physics"
	 * vs. control surfaces and are the final answer
	 * as to what we will render in the next pass
	*/
	unsigned int x;
	unsigned int y;
	unsigned int angle;
	unsigned int pitch;
	unsigned int input;

	int velocity;
	int throttle_time;

	struct chopper_gfx *gfx;
};

static LIST_HEAD(gfx_list);

static int load_pitch(struct chopper_gfx *gfx, unsigned int angle,
			unsigned int pitch)
{
	char buf[512];

	snprintf(buf, sizeof(buf), "data/chopper/apache_%d_%d.png",
		angle, pitch);

	gfx->angle[angle].pitch[pitch] = png_get_by_name(buf);
	if ( NULL == gfx->angle[angle].pitch[pitch] )
		return 0;

	return 1;
}

static int load_bank(struct chopper_gfx *gfx, unsigned int angle,
			unsigned int bank)
{
	char buf[512];

	snprintf(buf, sizeof(buf), "data/chopper/apache_%d_%s.png",
		angle, (bank) ? "bl" : "br");

	gfx->angle[angle].bank[bank] = png_get_by_name(buf);
	if ( NULL == gfx->angle[angle].bank[bank] )
		return 0;

	return 1;
}

static int load_angle(struct chopper_gfx *gfx, unsigned int angle)
{
	unsigned int i;

	for(i = 0; i < CHOPPER_NUM_PITCHES; i++) {
		if ( !load_pitch(gfx, angle, i) )
			return 0;
	}

	for(i = 0; i < CHOPPER_NUM_BANKS; i++) {
		if ( !load_bank(gfx, angle, i) )
			return 0;
	}

	return 1;
}

static struct chopper_gfx *gfx_get(const char *name)
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

	gfx->name = strdup(name);
	if ( NULL == gfx->name )
		goto out_free;

	for(i = 0; i < CHOPPER_NUM_ANGLES; i++) {
		if ( !load_angle(gfx, i) )
			goto out_free_all;
	}

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

	free(gfx->name);
	free(gfx);
}

static void gfx_put(struct chopper_gfx *gfx)
{
	gfx->refcnt--;
	if ( !gfx->refcnt )
		gfx_free(gfx);
}

static chopper_t get_chopper(const char *name)
{
	struct _chopper *c = NULL;

	c = calloc(1, sizeof(*c));
	if ( NULL == c )
		goto out;

	c->gfx = gfx_get(name);
	if ( NULL == c->gfx )
		goto out_free;

	c->angle = 6;
	c->pitch = 0;
	c->x = 0;
	c->y = 480;

	/* success */
	goto out;

out_free:
	free(c);
	c = NULL;
out:
	return c;
}

chopper_t chopper_apache(void)
{
	return get_chopper("apache");
}

chopper_t chopper_comanche(void)
{
	return get_chopper("comanche");
}

void chopper_render(chopper_t chopper, game_t g)
{
	texture_t tex;
	SDL_Rect dst;

	tex = chopper->gfx->angle[chopper->angle].pitch[chopper->pitch];

	dst.x = chopper->x;
	dst.y = chopper->y;
	dst.h = texture_height(tex);
	dst.y = texture_width(tex);

	game_blit(g, tex, NULL, &dst);
}

void chopper_free(chopper_t chopper)
{
	if ( chopper ) {
		gfx_put(chopper->gfx);
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

	chopper->velocity = chopper->throttle_time * VELOCITY_INCREMENTS;

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

	chopper->x += chopper->velocity;
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
