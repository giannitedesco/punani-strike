/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_GAME_H
#define _PUNANI_GAME_H

#include <punani/tex.h>

typedef struct _game *game_t;

/* lifetime */
game_t game_new(renderer_t render);
void game_free(game_t g);

/* state machine */
#define GAME_STATE_STOPPED	0
#define GAME_STATE_LOBBY	1
#define GAME_STATE_ON		2
#define GAME_NUM_STATES		3
unsigned int game_state(game_t g);
int game_start(game_t g);
void game_exit(game_t g);

/* time */
void game_new_frame(game_t g);
void game_render(game_t g, float lerp);

/* input */
void game_mousemove(game_t g, int xrel, int yrel);
void game_keypress(game_t g, int key, int down);
void game_mousebutton(game_t g, int button, int down);

/* sigh, the renderer calls this to pass exit codes from our game
 * modes back through to the game, it's a bit roundabout but it's
 * the best design I could come up with
*/
void game_mode_exit(void *priv, int code);

#endif /* _PUNANI_GAME_H */
