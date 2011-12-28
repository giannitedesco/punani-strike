/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/game.h>
#include <punani/tex.h>

#include "game-modes.h"

struct _game {
	renderer_t g_render;
	void *g_priv;
	const struct game_ops *g_ops;
	unsigned int g_state;
};

static const struct game_ops *game_modes[GAME_NUM_STATES] = {
	[GAME_STATE_STOPPED] NULL,
	[GAME_STATE_LOBBY] &lobby_ops,
	[GAME_STATE_ON] &world_ops,
};

static int transition(struct _game *g, unsigned int state)
{
	if ( g->g_ops && g->g_priv ) {
		(*g->g_ops->dtor)(g->g_priv);
		g->g_priv = NULL;
	}

	g->g_ops = game_modes[state];
	if ( g->g_ops ) {
		g->g_priv = (*g->g_ops->ctor)(g->g_render);
		if ( NULL == g->g_priv )
			return 0;
	}

	g->g_state = state;
	return 1;
}

game_t game_new(renderer_t renderer)
{
	struct _game *g;

	g = calloc(1, sizeof(*g));
	if ( NULL == g )
		goto out;

	if ( !renderer_init(renderer, 960, 540, 24, 0) )
		goto out_free;

	g->g_render = renderer;

	/* success */
	goto out;

out_free:
	free(g);
	g = NULL;
out:
	return g;
}

unsigned int game_state(game_t g)
{
	return g->g_state;
}

void game_exit(game_t g)
{
	transition(g, GAME_STATE_STOPPED);
}

int game_lobby(game_t g)
{
	return transition(g, GAME_STATE_LOBBY);
}

int game_start(game_t g)
{
	return transition(g, GAME_STATE_ON);
}

void game_free(game_t g)
{
	if ( g ) {
		free(g);
	}
}

/* one tick has elapsed in game time. the game tick interval
 * is clamped to real time so we can increment the emulation
 * as accurately as possible to wall time
*/
void game_new_frame(game_t g)
{
	if ( g->g_ops && g->g_ops->new_frame )
		(*g->g_ops->new_frame)(g->g_priv);
}

/* lerp is a value clamped between 0 and 1 which indicates how far between
 * game ticks we are. render times may fluctuate but we are called to
 * render as fast as possible
*/
void game_render(game_t g, float lerp)
{
	if ( g->g_ops && g->g_ops->render )
		(*g->g_ops->render)(g->g_priv, lerp);
}

void game_keypress(game_t g, int key, int down)
{
	if ( NULL == g->g_ops || NULL == g->g_ops->keypress )
		return;
	(*g->g_ops->keypress)(g->g_priv, key, down);
}

void game_mousebutton(game_t g, int button, int down)
{
	if ( NULL == g->g_ops || NULL == g->g_ops->mousebutton )
		return;
	(*g->g_ops->mousebutton)(g->g_priv, button, down);
}

void game_mousemove(game_t g, unsigned int x, unsigned int y,
				int xrel, int yrel)
{
	if ( NULL == g->g_ops || NULL == g->g_ops->mousemove )
		return;
	(*g->g_ops->mousemove)(g->g_priv, x, y, xrel, yrel);
}

void game_mode_exit(void *priv, int code)
{
	struct _game *g = priv;
	if ( code == GAME_MODE_QUIT ) {
		game_exit(g);
		return;
	}

	assert(code == GAME_MODE_COMPLETE);

	switch(g->g_state) {
	case GAME_STATE_LOBBY:
		game_start(g);
		break;
	case GAME_STATE_ON:
		game_exit(g);
		break;
	default:
		abort();
	}
}

