#include <punani/punani.h>
#include <punani/game.h>
#include <punani/tex.h>

#include "game-modes.h"

struct lobby {
	game_t game;
};

static void *ctor(game_t g)
{
	struct lobby *lobby;

	lobby = calloc(1, sizeof(*lobby));
	if ( NULL == lobby )
		return NULL;

	lobby->game = g;

	return lobby;
}

static void dtor(void *priv)
{
	struct lobby *lobby = priv;
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
	.keypress = keypress,
};
