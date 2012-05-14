/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_MISSILE_H
#define _PUNANI_MISSILE_H

typedef struct _missile *missile_t;

missile_t missile_spawn(const vec3_t origin, float heading, float pitch);

#endif /* _PUNANI_MISSILE_H */
