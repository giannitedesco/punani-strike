/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_PARTICLES_H
#define _PUNANI_PARTICLES_H

typedef struct _particles *particles_t;

particles_t particles_new(unsigned int max);
void particles_think(particles_t p);
void particles_free(particles_t p);

void particles_emit(particles_t p, const vec3_t begin, const vec3_t end);

void particles_render_all(float lerp);
void particles_think_all(void);

#endif /* _PUNANI_PARTICLES_H */
