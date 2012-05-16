/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _ENTITY_INTERNAL_H
#define _ENTITY_INTERNAL_H

#include "list.h"

struct _entity {
	struct list_head e_list;
	struct list_head e_group;
	const struct entity_ops *e_ops;
	vec3_t e_origin;
	vec3_t e_move;
	vec3_t e_oldorigin;
	vec3_t e_oldlerp;
	unsigned int e_ref;
};

/* Mask to extract type from flags */
#define ENT_TYPE_BITS	1
#define ENT_TYPE_MASK	((1 << ENT_TYPE_BITS) - 1)
#define ENT_PROJECTILE	0
#define ENT_HELI	1

struct entity_ops {
	void (*e_render)(struct _entity *ent, renderer_t r,
				float lerp, light_t l);
	void (*e_think)(struct _entity *e);
	void (*e_dtor)(struct _entity *e);
	void (*e_collide_world)(struct _entity *ent, const vec3_t hit);
	unsigned int e_flags;
};

void entity_unref(struct _entity *ent);
void entity_render(struct _entity *ent, renderer_t r, float lerp, light_t l);
void entity_spawn(struct _entity *ent, const struct entity_ops *ops,
			const vec3_t origin, const vec3_t move);

static inline void entity_ref(struct _entity *ent)
{
	ent->e_ref++;
}

#endif /* _ENTITY_INTERNAL_H */
