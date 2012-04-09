/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/tex.h>

#include "game-modes.h"

#include <SDL/SDL_keysym.h>

struct lobby {
	renderer_t renderer;
	texture_t splash;
};

static void *ctor(renderer_t r, void *common)
{
	struct lobby *lobby = NULL;

	lobby = calloc(1, sizeof(*lobby));
	if ( NULL == lobby )
		return NULL;

	lobby->renderer = r;

	lobby->splash = png_get_by_name(r, "data/splash.png", 0);
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
	renderer_t r = lobby->renderer;
	prect_t dst;

	renderer_render_2d(r);
	renderer_clear_color(r, 0.0, 0.0, 0.0);

	renderer_size(r, &x, &y);
	sx = texture_width(lobby->splash);
	sy = texture_height(lobby->splash);

	dst.x = (x - sx) / 2;
	dst.y = (y - sy) / 2;
	dst.w = sx;
	dst.h = sy;

	renderer_blit(r, lobby->splash, NULL, &dst);
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
	if ( !down )
		return;
	switch(key) {
	case SDLK_RETURN:
		renderer_exit(lobby->renderer, GAME_MODE_COMPLETE);
		break;
	case SDLK_ESCAPE:
		renderer_exit(lobby->renderer, GAME_MODE_QUIT);
		break;
	default:
		break;
	}
}

const struct game_ops lobby_ops = {
	.ctor = ctor,
	.dtor = dtor,
	.render = render,
	.keypress = keypress,
};
