/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/vec.h>
#include <punani/entity.h>
#include "ent-internal.h"

static LIST_HEAD(ents);

void entity_link(entity_t ent)
{
	list_add_tail(&ent->e_list, &ents);
	ent->e_ref++;
}

void entity_unref(entity_t ent)
{
	ent->e_ref--;
	if ( !ent->e_ref ) {
		(*ent->e_ops->e_dtor)(ent);
	}
}

void entity_unlink(struct _entity *ent)
{
	list_del(&ent->e_list);
	entity_unref(ent);
}

void entity_spawn(struct _entity *ent, const struct entity_ops *ops,
			const vec3_t origin, const vec3_t move)
{
	memset(ent, 0, sizeof(*ent));
	ent->e_ops = ops;
	v_copy(ent->e_origin, origin);
	v_copy(ent->e_oldorigin, origin);
	v_copy(ent->e_oldlerp, origin);
	if ( move )
		v_copy(ent->e_move, move);
}

void entity_think(struct _entity *ent)
{
	if ( ent->e_ops->e_think )
		(*ent->e_ops->e_think)(ent);
}

void entity_think_all(void)
{
	entity_t ent, tmp;

	list_for_each_entry_safe(ent, tmp, &ents, e_list) {
		entity_think(ent);
	}
}

void entity_render(struct _entity *ent, renderer_t r, float lerp, light_t l)
{
	if ( ent->e_ops->e_render )
		(*ent->e_ops->e_render)(ent, r, lerp, l);
}

void entity_render_all(renderer_t r, float lerp, light_t l)
{
	entity_t ent, tmp;

	list_for_each_entry_safe(ent, tmp, &ents, e_list) {
		entity_render(ent, r, lerp, l);
	}
}
