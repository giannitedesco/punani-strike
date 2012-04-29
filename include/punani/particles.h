/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_PARTICLES_H
#define _PUNANI_PARTICLES_H

typedef struct _particles *particles_t;

particles_t particles_new(unsigned int max);
void particles_think(particles_t p);
void particles_render(particles_t p, float lerp);
void particles_free(particles_t p);

#endif /* _PUNANI_PARTICLES_H */
