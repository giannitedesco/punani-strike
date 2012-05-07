/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/game.h>
#include <punani/tex.h>
#include <punani/console.h>


#include "game-modes.h"
#include "render-internal.h"

struct _game {
	renderer_t g_render;
	void *g_priv;
	unsigned int g_state;
	const struct game_ops *g_ops;
	const struct game_ops * const *g_modes;
	unsigned int g_num_modes;
	game_exit_fn_t g_efn;
	void *g_common;
	texture_t con_back;
	font_t con_font;
};

int game_set_state(struct _game *g, unsigned int state)
{
	const struct game_ops *ops;
	assert(state < g->g_num_modes);

	ops = g->g_modes[state];
	if ( ops ) {
		void *priv;
		priv = (*ops->ctor)(g->g_render, g->g_common);
		if ( NULL == priv ) {
			return 0;
		}
		if ( g->g_ops && g->g_priv ) {
			(*g->g_ops->dtor)(g->g_priv);
		}
		g->g_priv = priv;
	}else{
		if ( g->g_ops && g->g_priv ) {
			(*g->g_ops->dtor)(g->g_priv);
		}
		g->g_priv = NULL;
	}

	g->g_ops = ops;
	g->g_state = state;
	return 1;
}

struct _game *game_new(const struct game_ops * const *modes,
		unsigned int num_modes, game_exit_fn_t efn, void *priv)
{
	struct _game *g;

	assert(num_modes);
	assert(modes[0] == NULL);

	g = calloc(1, sizeof(*g));
	if ( NULL == g )
		goto out;

	g->g_modes = modes;
	g->g_num_modes = num_modes;
	g->g_efn = efn;
	g->g_common = priv;

	g->g_render = renderer_new(g);
	if ( NULL == g->g_render )
		goto out_free;

	/* success */
	goto out;

out_free:
	free(g);
	g = NULL;
out:
	return g;
}

int game_mode(game_t g, const char *title,
			unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen)
{
	int result;
	
	result = renderer_mode(g->g_render, title, x, y, depth, fullscreen);
	
	if ( NULL == g->con_font ) {
		g->con_font = font_load(g->g_render, "data/font/acknowtt.png", 12, 16);
		if ( NULL != g->con_font ) {
			g->con_back = png_get_by_name(g->g_render, "data/conback.png");
			// will work fine without one, so don't bother to test.
				
			con_init(g->con_font, g->con_back, x, y);
		}
	}
	
	return result;
}

unsigned int game_state(game_t g)
{
	return g->g_state;
}

void game_exit(game_t g)
{
	game_set_state(g, GAME_STATE_STOPPED);
}

void game_free(game_t g)
{
	if ( g ) {
		renderer_free(g->g_render);
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
		
	con_render();
}

void game_keypress(game_t g, int key, int down)
{
	/* let the console have first dibs - we might be typing into it or hitting the key to show it. */
	if (con_keypress(key, down)) return;
		
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

	(*g->g_efn)(g, code);
}

int game_main(game_t g)
{
	return renderer_main(g->g_render);
}
