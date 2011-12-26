#include <punani/punani.h>
#include <punani/game.h>
#include <punani/tex.h>

#include "game-modes.h"

struct world {
	game_t game;
	texture_t map;
};

static void *ctor(game_t g)
{
	struct world *world = NULL;

	world = calloc(1, sizeof(*world));
	if ( NULL == world )
		goto out;

	world->game = g;

	world->map = png_get_by_name("data/map1.png");
	if ( NULL == world->map )
		goto out_free;

	/* success */
	goto out;

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
}

static void dtor(void *priv)
{
	struct world *world = priv;
	// texture_put(world->map);
	free(world);
}

static void keypress(void *priv, int key, int down)
{
	//struct world *world = priv;
}

const struct game_ops world_ops = {
	.ctor = ctor,
	.dtor = dtor,
	.render = render,
	.keypress = keypress,
};
