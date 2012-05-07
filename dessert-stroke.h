#ifndef _DESSERT_STROKE_H
#define _DESSERT_STROKE_H

#define DS_STATE_LOBBY	1
#define DS_STATE_ON	2
#define DS_NUM_STATES	3

extern const struct game_ops lobby_ops;
extern const struct game_ops world_ops;

#define TILE_X 25.0f
#define TILE_Y 25.0f

#endif /* _DESSERT_STROKE */
