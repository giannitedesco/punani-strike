/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/punani_gl.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/vec.h>
#include <punani/map.h>
#include <punani/entity.h>
#include "ent-internal.h"

static LIST_HEAD(ents);
static LIST_HEAD(projectiles);
static LIST_HEAD(helis);

void entity_link(entity_t ent)
{
	list_add_tail(&ent->e_list, &ents);
	switch(ent->e_ops->e_flags & ENT_TYPE_MASK) {
	case ENT_PROJECTILE:
		list_add_tail(&ent->e_group, &projectiles);
		break;
	case ENT_HELI:
		list_add_tail(&ent->e_group, &helis);
		break;
	default:
		abort();
		break;
	}
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
	list_del(&ent->e_group);
	entity_unref(ent);
}

void entity_spawn(struct _entity *ent, const struct entity_ops *ops,
			const vec3_t origin, const vec3_t move)
{
	memset(ent, 0, sizeof(*ent));
	ent->e_ops = ops;
	v_copy(ent->e_origin, origin);
	v_copy(ent->e_lerp, origin);
	v_copy(ent->e_oldorigin, origin);
	v_copy(ent->e_oldlerp, origin);
	if ( move )
		v_copy(ent->e_move, move);
}

static int entity_collide_world(struct _entity *ent, map_t map, vec3_t hit)
{
	switch(ent->e_ops->e_flags & ENT_TYPE_MASK) {
	case ENT_PROJECTILE:
		return map_collide_line(map, ent->e_oldorigin,
					ent->e_origin, hit);
		break;
	case ENT_HELI:
		break;
	default:
		abort();
		break;
	}
	return 0;
}

static void entity_think(struct _entity *ent, map_t map)
{
	vec3_t hit;

	if ( ent->e_ops->e_think )
		(*ent->e_ops->e_think)(ent);

	if ( !entity_collide_world(ent, map, hit) )
		return;

	if ( ent->e_ops->e_collide_world )
		(*ent->e_ops->e_collide_world)(ent, hit);
}

void entity_think_all(map_t map)
{
	entity_t ent, tmp;

	list_for_each_entry_safe(ent, tmp, &ents, e_list) {
		entity_think(ent, map);
	}
}

void entity_render(struct _entity *ent, renderer_t r, float lerp, light_t l)
{
	if ( NULL == ent->e_ops->e_render )
		return;

	v_copy(ent->e_oldlerp, ent->e_lerp);
	ent->e_lerp[0] = ent->e_oldorigin[0] + ent->e_move[0] * lerp;
	ent->e_lerp[1] = ent->e_oldorigin[1] + ent->e_move[1] * lerp;
	ent->e_lerp[2] = ent->e_oldorigin[2] + ent->e_move[2] * lerp;

	glPushMatrix();
	renderer_translate(r, ent->e_lerp[0], ent->e_lerp[1], ent->e_lerp[2]);
	(*ent->e_ops->e_render)(ent, r, lerp, l);
	glPopMatrix();
}

void entity_render_all(renderer_t r, float lerp, light_t l)
{
	entity_t ent, tmp;

	list_for_each_entry_safe(ent, tmp, &ents, e_list) {
		entity_render(ent, r, lerp, l);
	}
}
