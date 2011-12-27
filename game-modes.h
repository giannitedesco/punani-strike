/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _GAME_OPS_H
#define _GAME_OPS_H

#define GAME_MODE_COMPLETE	0
#define GAME_MODE_QUIT		1

struct game_ops {
	/* lifetime */
	void *(*ctor)(renderer_t r);
	void (*dtor)(void *);

	/* time */
	void (*new_frame)(void *);
	void (*render)(void *, float lerp);

	/* input */
	void (*mousemove)(void *, int xrel, int yrel);
	void (*keypress)(void *, int key, int down);
	void (*mousebutton)(void *, int button, int down);
};

extern const struct game_ops lobby_ops;
extern const struct game_ops world_ops;

#endif /* _GAME_OPS_H */
