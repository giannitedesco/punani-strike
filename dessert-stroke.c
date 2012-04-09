/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/game.h>
#include <punani/renderer.h>
#include <punani/tex.h>

#include "dessert-stroke.h"
#include "game-modes.h"

#define DS_STATE_LOBBY	1
#define DS_STATE_ON	2
#define DS_NUM_STATES	3

static const struct game_ops *game_modes[DS_NUM_STATES] = {
	[GAME_STATE_STOPPED] NULL,
	[DS_STATE_LOBBY] &lobby_ops,
	[DS_STATE_ON] &world_ops,
};

static void mode_exit(struct _game *g, int code)
{
	switch(game_state(g)) {
	case DS_STATE_LOBBY:
		game_set_state(g, DS_STATE_ON);
		break;
	case DS_STATE_ON:
		game_exit(g);
		break;
	default:
		abort();
	}
}

int main(int argc, char **argv)
{
	game_t g;

	g = game_new(game_modes, DS_NUM_STATES, mode_exit, NULL);
	if ( NULL == g )
		return EXIT_FAILURE;

	if ( !game_mode(g, "Dessert Stroke", 960, 540, 24, 0) )
		return EXIT_FAILURE;

	game_set_state(g, DS_STATE_LOBBY);
	if ( !game_main(g) )
		return EXIT_FAILURE;

	game_free(g);
	return EXIT_SUCCESS;
}
