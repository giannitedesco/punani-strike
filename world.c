#include <punani/punani.h>
#include <punani/game.h>
#include <punani/tex.h>
#include <punani/chopper.h>

#include "game-modes.h"

struct world {
	game_t game;
	texture_t map;
	chopper_t apache;
};

static void *ctor(game_t g)
{
	struct world *world = NULL;

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

static void render(void *priv, float lerp)
{
	struct world *world = priv;
	game_t g = world->game;

	game_blit(g, world->map, NULL, NULL);
	chopper_render(world->apache, g);
}

static void dtor(void *priv)
{
	struct world *world = priv;
	chopper_free(world->apache);
	texture_put(world->map);
	free(world);
}

static void keypress(void *priv, int key, int down)
{
	struct world *world = priv;
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
	struct world *world = priv;
	chopper_think(world->apache);
}

const struct game_ops world_ops = {
	.ctor = ctor,
	.dtor = dtor,
	.new_frame = frame,
	.render = render,
	.keypress = keypress,
};
