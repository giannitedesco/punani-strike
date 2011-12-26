#ifndef _GAME_OPS_H
#define _GAME_OPS_H

struct game_ops {
	/* lifetime */
	void *(*ctor)(game_t g);
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
