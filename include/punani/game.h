#ifndef _PUNANI_GAME_H
#define _PUNANI_GAME_H

typedef struct _game *game_t;

#define GAME_STATE_STOPPED	0
#define GAME_STATE_LOBBY	1
#define GAME_STATE_ON		2

game_t game_new(void);
unsigned int game_state(game_t g);
void game_new_frame(game_t g);
void game_render(game_t g, float lerp);
int game_start(game_t g);
void game_exit(game_t g);
void game_free(game_t g);

#endif /* _PUNANI_GAME_H */
