/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/game.h>
#include <punani/renderer.h>
#include <punani/map.h>
#include <punani/tex.h>

#include "dessert-stroke.h"
#include "game-modes.h"

#include <SDL/SDL_keysym.h>

struct tiledit_common {
	const char *fn;
};

struct tiledit {
	renderer_t renderer;
	map_t map;
};

static void *ctor(renderer_t r, void *priv)
{	
	struct tiledit_common *common = priv;
	struct tiledit *tiledit = NULL;

	tiledit = calloc(1, sizeof(*tiledit));
	if ( NULL == tiledit )
		return NULL;

	tiledit->renderer = r;

	tiledit->map = map_load(common->fn);
	if ( NULL == tiledit->map )
		goto out_free;

	/* success */
	goto out;

out_free:
	free(tiledit);
	tiledit = NULL;
out:
	return tiledit;
}

static void render(void *priv, float lerp)
{
	struct tiledit *tiledit = priv;
	unsigned int sx, sy;
	prect_t src;

	renderer_size(tiledit->renderer, &sx, &sy);

	src.x = 0;
	src.y = 0;
	src.w = sx;
	src.h = sy;

	map_render(tiledit->map, tiledit->renderer, &src);
}

static void dtor(void *priv)
{
	struct tiledit *tiledit = priv;
	map_free(tiledit->map);
	free(tiledit);
}

static void keypress(void *priv, int key, int down)
{
	struct tiledit *tiledit = priv;
	switch(key) {
	case SDLK_ESCAPE:
	case SDLK_q:
		renderer_exit(tiledit->renderer, GAME_MODE_COMPLETE);
		break;
	default:
		break;
	}
}

const struct game_ops tiledit_ops = {
	.ctor = ctor,
	.dtor = dtor,
	.render = render,
	.keypress = keypress,
};
#define TILEDIT 1

static const struct game_ops *game_modes[2] = {
	[GAME_STATE_STOPPED] NULL,
	[TILEDIT] &tiledit_ops,
};

static void mode_exit(struct _game *g, int code)
{
	switch(game_state(g)) {
	case TILEDIT:
		game_exit(g);
		break;
	default:
		abort();
	}
}

int main(int argc, char **argv)
{
	struct tiledit_common common;
	game_t g;

	if ( argc < 2 ) {
		fprintf(stderr, "Usage:\t%s <filename>\n", argv[0]);
		return EXIT_FAILURE;
	}
	common.fn = argv[1];

	g = game_new("SDL", game_modes, 2, mode_exit, &common);
	if ( NULL == g )
		return EXIT_FAILURE;

	game_set_state(g, TILEDIT);

	if ( !game_main(g) )
		return EXIT_FAILURE;

	game_free(g);
	return EXIT_SUCCESS;
}
