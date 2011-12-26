#ifndef _PUNANI_WORLD_H
#define _PUNANI_WORLD_H

typedef struct _world *world_t;

void world_blit(world_t world, texture_t tex, SDL_Rect *src, SDL_Rect *dst);

#endif /* _PUNANI_WORLD_H */
