/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_TILE_H
#define _PUNANI_TILE_H

typedef struct _light *light_t;

light_t light_new(void);
void light_render(light_t l);
void light_free(light_t l);

#endif /* _PUNANI_TILE_H */
