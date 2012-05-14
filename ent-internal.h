/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _ENTITY_INTERNAL_H
#define _ENTITY_INTERNAL_H

#include "list.h"

struct _entity {
	struct list_head e_list;
	const struct entity_ops *e_ops;
	vec3_t e_origin;
	vec3_t e_move;
	vec3_t e_oldorigin;
	vec3_t e_oldlerp;
	unsigned int e_flags;
	unsigned int e_ref;
};

struct entity_ops {
	void (*e_render)(struct _entity *ent, renderer_t r,
				float lerp, light_t l);
	void (*e_think)(struct _entity *e);
	void (*e_dtor)(struct _entity *e);
};

void entity_unref(struct _entity *ent);
void entity_think(struct _entity *ent);
void entity_render(struct _entity *ent, renderer_t r, float lerp, light_t l);
void entity_spawn(struct _entity *ent, const struct entity_ops *ops,
			const vec3_t origin, const vec3_t move);

static inline void entity_ref(struct _entity *ent)
{
	ent->e_ref++;
}

#endif /* _ENTITY_INTERNAL_H */
