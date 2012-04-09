/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _RENDERER_INTERNAL_H
#define _RENDERER_INTERNAL_H

const struct tex_ops *renderer_texops(struct _renderer *r);

#if RENDER_LIGHTS
int renderer_get_free_light(renderer_t r);
void renderer_set_light(renderer_t r, unsigned int num, light_t l);
void renderer_nuke_light(renderer_t r, unsigned int num);

void light_render(light_t l);
int light_enabled(light_t l);
#endif

#endif /* _RENDERER_INTERNAL_H */
