#include <punani/punani.h>
#include <punani/game.h>
#include <punani/tex.h>
#include <punani/chopper.h>
#include "list.h"

#define CHOPPER_NUM_ANGLES	12
#define CHOPPER_NUM_PITCHES	4
#define CHOPPER_NUM_BANKS	2

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
	unsigned int x;
	unsigned int y;
	unsigned int angle;
	unsigned int pitch;
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
	c->x = 640;
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

	tex = chopper->gfx->angle[chopper->angle].pitch[chopper->pitch];
	game_blit(g, tex, NULL, NULL);
}

void chopper_free(chopper_t chopper)
{
	if ( chopper ) {
		gfx_put(chopper->gfx);
		free(chopper);
	}
}
