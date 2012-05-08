/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_CHOPPER_H
#define _PUNANI_CHOPPER_H

typedef struct _chopper *chopper_t;

#define CHOPPER_THROTTLE	0
#define CHOPPER_BRAKE		1
#define CHOPPER_LEFT		2
#define CHOPPER_RIGHT		3

chopper_t chopper_comanche(const vec3_t pos, float h);
void chopper_get_pos(chopper_t chopper, float lerp, vec3_t out);
void chopper_think(chopper_t chopper);
void chopper_render(chopper_t chopper, renderer_t r, float lerp, light_t l);
void chopper_render_missiles(chopper_t chopper, renderer_t r,
				float lerp, light_t l);
void chopper_control(chopper_t chopper, unsigned int ctrl, int down);
void chopper_control_release_all(chopper_t chopper);
void chopper_fire(chopper_t chopper, renderer_t r, unsigned int time);
void chopper_free(chopper_t chopper);

#endif /* _PUNANI_CHOPPER_H */
