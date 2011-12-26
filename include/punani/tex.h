#ifndef _PUNANI_TEX_H
#define _PUNANI_TEX_H

typedef struct _texture *texture_t;

texture_t png_get_by_name(const char *name, unsigned int xflip);

void texture_put(texture_t t);
unsigned int texture_width(texture_t t);
unsigned int texture_height(texture_t t);

SDL_Surface *texture_surface(texture_t tex);

#ifdef _PUNANI_GAME_H
void game_blit(game_t g, texture_t tex, SDL_Rect *src, SDL_Rect *dst);
#endif

#endif /* _PUNANI_TEX_H */
