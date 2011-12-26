/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/game.h>
#include <punani/tex.h>

#include "game-modes.h"

struct lobby {
	game_t game;
	texture_t splash;
};

static void *ctor(game_t g)
{
	struct lobby *lobby = NULL;

	lobby = calloc(1, sizeof(*lobby));
	if ( NULL == lobby )
		return NULL;

	lobby->game = g;

	lobby->splash = png_get_by_name("data/splash.png", 0);
	if ( NULL == lobby->splash )
		goto out_free;

	/* success */
	goto out;

out_free:
	free(lobby);
	lobby = NULL;
out:
	return lobby;
}

static void render(void *priv, float lerp)
{
	struct lobby *lobby = priv;
	unsigned int x, y, sx, sy;
	game_t g = lobby->game;
	SDL_Rect dst;

	game_screen_size(g, &x, &y);
	sx = texture_width(lobby->splash);
	sy = texture_height(lobby->splash);

	dst.x = (x - sx) / 2;
	dst.y = (y - sy) / 2;
	dst.w = sx;
	dst.h = sy;

	game_blit(g, lobby->splash, NULL, &dst);
}

static void dtor(void *priv)
{
	struct lobby *lobby = priv;
	texture_put(lobby->splash);
	free(lobby);
}

static void keypress(void *priv, int key, int down)
{
	struct lobby *lobby = priv;
	if ( key == SDLK_RETURN )
		game_start(lobby->game);
}

const struct game_ops lobby_ops = {
	.ctor = ctor,
	.dtor = dtor,
	.render = render,
	.keypress = keypress,
};
