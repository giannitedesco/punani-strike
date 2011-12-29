/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _GAME_OPS_H
#define _GAME_OPS_H

#define GAME_MODE_COMPLETE	0
#define GAME_MODE_QUIT		1

struct _game;

struct game_ops {
	/* lifetime */
	void *(*ctor)(renderer_t r, void *priv);
	void (*dtor)(void *);

	/* time */
	void (*new_frame)(void *);
	void (*render)(void *, float lerp);

	/* input */
	void (*keypress)(void *, int key, int down);
	void (*mousebutton)(void *, int button, int down);
	void (*mousemove)(void *, unsigned int x, unsigned int y,
				int xrel, int yrel);
};

typedef void (*game_exit_fn_t)(struct _game *g, int code);
struct _game *game_new(const char *renderer,
			const struct game_ops * const *modes,
			unsigned int num_modes,
			game_exit_fn_t efn,
			void *priv);

#endif /* _GAME_OPS_H */
