/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/world.h>
#include <punani/map.h>
#include <punani/chopper.h>
#include <punani/light.h>

#include "game-modes.h"
#include "render-internal.h"

#include <SDL/SDL_keysym.h>

struct _world {
	renderer_t render;
	map_t map;
	chopper_t apache;
	light_t light;
};

static void *ctor(renderer_t r, void *common)
{
	struct _world *world = NULL;

	world = calloc(1, sizeof(*world));
	if ( NULL == world )
		goto out;

	world->render = r;

	world->map = map_load(r, "data/map/1.psm");
	if ( NULL == world->map )
		goto out_free;

	world->apache = chopper_apache(r, 562, 1994, 0.785);
	if ( NULL == world->apache )
		goto out_free_map;

	world->light = light_new();
	if ( NULL == world->light ) {
		goto out_free_chopper;
	}

	/* success */
	goto out;

out_free_chopper:
	chopper_free(world->apache);
out_free_map:
	map_free(world->map);
out_free:
	free(world);
	world = NULL;
out:
	return world;
}

static void render_hud(void *priv, float lerp)
{
	struct _world *world = priv;
	chopper_render(world->apache, world->render, lerp);
}

static void render_3d(void *priv, float lerp)
{
	struct _world *world = priv;
	unsigned int x, y;
	unsigned int sx, sy;
	unsigned int cx, cy;

	/* try to keep chopper at centre of screen */
	chopper_pre_render(world->apache, lerp);
	chopper_get_pos(world->apache, &x, &y);
	chopper_get_size(world->apache, &cx, &cy);
	renderer_size(world->render, &sx, &sy);

	light_render(world->light);
	map_render(world->map, world->render);
}

static void dtor(void *priv)
{
	struct _world *world = priv;
	light_free(world->light);
	chopper_free(world->apache);
	map_free(world->map);
	free(world);
}

static void keypress(void *priv, int key, int down)
{
	struct _world *world = priv;
	switch(key) {
	case SDLK_LEFT:
		chopper_control(world->apache, CHOPPER_LEFT, down);
		break;
	case SDLK_RIGHT:
		chopper_control(world->apache, CHOPPER_RIGHT, down);
		break;
	case SDLK_UP:
		chopper_control(world->apache, CHOPPER_THROTTLE, down);
		break;
	case SDLK_DOWN:
		chopper_control(world->apache, CHOPPER_BRAKE, down);
		break;
	case SDLK_q:
	case SDLK_ESCAPE:
		renderer_exit(world->render, GAME_MODE_COMPLETE);
		break;
	default:
		break;
	}
}

static void frame(void *priv)
{
	struct _world *world = priv;
	chopper_think(world->apache);
}

const struct game_ops world_ops = {
	.ctor = ctor,
	.dtor = dtor,
	.new_frame = frame,
	.render_hud = render_hud,
	.render_3d = render_3d,
	.keypress = keypress,
};
