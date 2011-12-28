/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/tex.h>
#include <punani/renderer.h>
#include <punani/world.h>
#include <punani/map.h>
#include <punani/chopper.h>

#include "game-modes.h"
#include "render-internal.h"

#include <SDL/SDL_keysym.h>

struct _world {
	renderer_t render;
	map_t map;
	chopper_t apache;

	/* top-left-most pixel to render */
	unsigned int x;
	unsigned int y;
};

static void *ctor(renderer_t r)
{
	struct _world *world = NULL;

	world = calloc(1, sizeof(*world));
	if ( NULL == world )
		goto out;

	world->render = r;

	world->map = map_load("data/map/1.psm");
	if ( NULL == world->map )
		goto out_free;

	world->apache = chopper_apache(562, 1994, 0.785);
	if ( NULL == world->apache )
		goto out_free_map;

	/* success */
	goto out;

out_free_map:
	map_free(world->map);
out_free:
	free(world);
	world = NULL;
out:
	return world;
}

static void r_render(void *priv, texture_t tex, prect_t *src, prect_t *dst)
{
	world_t world = priv;
	prect_t d;

	d.x = - world->x;
	d.y = - world->y;
	if ( NULL == src ) {
		d.w = texture_width(tex);
		d.h = texture_height(tex);
	}

	if ( dst ) {
		d.x += dst->x;
		d.y += dst->y;
		d.w += dst->w;
		d.h += dst->h;
	}

	renderer_blit(world->render, tex, src, &d);
}

static const struct render_ops r_ops = {
	.blit = r_render,
};

static void render(void *priv, float lerp)
{
	struct _world *world = priv;
	unsigned int x, y;
	unsigned int sx, sy;
	unsigned int cx, cy;
	unsigned int dx, dy;
	unsigned int mx, my;
	prect_t src;
	struct _renderer r = {
		.ops = &r_ops,
		.priv = world,
	};

	/* try to keep chopper at centre of screen */
	chopper_pre_render(world->apache, lerp);
	chopper_get_pos(world->apache, &x, &y);
	chopper_get_size(world->apache, &cx, &cy);
	renderer_size(world->render, &sx, &sy);

	if ( (int)cx < 0 )
		cx = 0;
	if ( (int)cy < 0 )
		cy = 0;

	map_get_size(world->map, &mx, &my);

	dx = (sx - cx) / 2;
	dy = (sy - cy) / 2;

	src.x = (dx > x) ? 0 : (x - dx);
	src.y = (dy > y) ? 0 : (y - dy);
	if ( (int)mx - src.x < (int)sx )
		src.x = mx - sx;
	if ( (int)my - src.y < (int)sy )
		src.y = my - sy;
	src.w = sx;
	src.h = sy;

	if ( (int)src.y < 0 )
		src.y = 0;
	if ( (int)src.x < 0 )
		src.x = 0;

	world->x = src.x;
	world->y = src.y;

	map_render(world->map, world->render, &src);
	chopper_render(world->apache, &r, lerp);
}

static void dtor(void *priv)
{
	struct _world *world = priv;
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
	.render = render,
	.keypress = keypress,
};
