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

chopper_t chopper_apache(unsigned int x, unsigned int y, float h);
chopper_t chopper_comanche(unsigned int x, unsigned int y, float h);
void chopper_get_size(chopper_t chopper, unsigned int *x, unsigned int *y);
void chopper_get_pos(chopper_t chopper, unsigned int *x, unsigned int *y);
void chopper_think(chopper_t chopper);
void chopper_pre_render(chopper_t chopper, float lerp);
void chopper_render(chopper_t chopper, renderer_t r, float lerp);
void chopper_control(chopper_t chopper, unsigned int ctrl, int down);
void chopper_free(chopper_t chopper);

#endif /* _PUNANI_CHOPPER_H */
