/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_ENTITY_H
#define _PUNANI_ENTITY_H

typedef struct _entity *entity_t;

void entity_link(entity_t ent);
void entity_unlink(entity_t ent);
void entity_think_all(void);
void entity_render_all(renderer_t r, float lerp, light_t l);

#endif /* _PUNANI_ENTITY_H */
