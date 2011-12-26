#include <punani/punani.h>
#include <punani/game.h>
#include <punani/tex.h>
#include <punani/world.h>
#include <punani/chopper.h>

#include "game-modes.h"

struct _world {
	game_t game;
	texture_t map;
	chopper_t apache;

	/* top-left-most pixel to render */
	unsigned int x;
	unsigned int y;
};

static void *ctor(game_t g)
{
	struct _world *world = NULL;

	world = calloc(1, sizeof(*world));
	if ( NULL == world )
		goto out;

	world->game = g;

	world->map = png_get_by_name("data/map/1.png", 0);
	if ( NULL == world->map )
		goto out_free;

	world->apache = chopper_apache();
	if ( NULL == world->apache )
		goto out_free_map;

	/* success */
	goto out;

out_free_map:
	texture_put(world->map);
out_free:
	free(world);
	world = NULL;
out:
	return world;
}

void world_blit(world_t world, texture_t tex, SDL_Rect *src, SDL_Rect *dst)
{
	SDL_Rect d;

	d.x = - world->x;
	d.y = - world->y;

	if ( dst ) {
		d.x += dst->x;
		d.y += dst->y;
		d.w += dst->w;
		d.h += dst->h;
	}

	game_blit(world->game, tex, src, &d);
}

static void render(void *priv, float lerp)
{
	struct _world *world = priv;
	game_t g = world->game;
	unsigned int x, y;
	unsigned int sx, sy;
	unsigned int cx, cy;
	unsigned int dx, dy;
	int mx, my;
	SDL_Rect src;

	/* try to keep chopper at centre of screen */
	chopper_get_pos(world->apache, &x, &y);
	chopper_get_size(world->apache, &cx, &cy);
	game_screen_size(g, &sx, &sy);

	mx = texture_width(world->map);
	my = texture_height(world->map);

	dx = (sx - cx) / 2;
	dy = (sy - cy) / 2;

	src.x = (dx > x) ? 0 : (x - dx);
	src.y = (dy > y) ? 0 : (y - dy);
	if ( mx - src.x < (int)sx )
		src.x = mx - sx;
	if ( my - src.y < (int)sy )
		src.y = my - sy;
	src.w = sx;
	src.h = sy;

	world->x = src.x;
	world->y = src.y;

	game_blit(g, world->map, &src, NULL);
	chopper_render(world->apache, world);
}

static void dtor(void *priv)
{
	struct _world *world = priv;
	chopper_free(world->apache);
	texture_put(world->map);
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
		game_exit(world->game);
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
