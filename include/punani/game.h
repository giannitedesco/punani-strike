/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_GAME_H
#define _PUNANI_GAME_H

typedef struct _game *game_t;

/* lifetime */
int game_main(game_t g);
void game_free(game_t g);

/* video */
int game_mode(game_t g, const char *title,
			unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen);

/* state machine */
#define GAME_STATE_STOPPED	0
unsigned int game_state(game_t g);
int game_set_state(game_t g, unsigned int state);
void game_exit(game_t g);

/* time */
void game_new_frame(game_t g);
void game_render(game_t g, float lerp);

/* input */
void game_mousemove(game_t g, unsigned int x, unsigned int y,
				int xrel, int yrel);
void game_keypress(game_t g, int key, int down, void *raw);
void game_mousebutton(game_t g, int button, int down);

void game_mode_exit(void *priv, int code);

#endif /* _PUNANI_GAME_H */
