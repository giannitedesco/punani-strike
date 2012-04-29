/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_LIGHT_H
#define _PUNANI_LIGHT_H

typedef struct _light *light_t;

#define LIGHT_CAST_SHADOWS	(1 << 0)

light_t light_new(renderer_t r, unsigned int flags);
void light_set_pos(light_t l, const vec3_t pos);
void light_get_pos(light_t l, vec3_t pos);
void light_set_color(light_t l, float r, float b, float g);
void light_disable(light_t l);
void light_enable(light_t l);
void light_render(light_t l);
void light_free(light_t l);

#endif /* _PUNANI_LIGHT_H */
