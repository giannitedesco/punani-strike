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
#include <punani/asset.h>
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
			const vec3_t origin, const vec3_t move,
			const vec3_t angles)
{
	memset(ent, 0, sizeof(*ent));
	ent->e_ops = ops;
	v_copy(ent->e_origin, origin);
	v_copy(ent->e_lerp, origin);
	v_copy(ent->e_oldorigin, origin);
	v_copy(ent->e_oldlerp, origin);
	if ( move )
		v_copy(ent->e_move, move);
	if ( angles ) {
		v_copy(ent->e_angles, angles);
		v_copy(ent->e_oldangles, angles);
	}
}

static void entity_think(struct _entity *ent, map_t map)
{
	vec3_t hit;

	v_copy(ent->e_oldorigin, ent->e_origin);
	v_copy(ent->e_oldangles, ent->e_angles);

	if ( ent->e_ops->e_think )
		(*ent->e_ops->e_think)(ent);

	v_add(ent->e_origin, ent->e_move);

	switch(ent->e_ops->e_flags & ENT_TYPE_MASK) {
	case ENT_PROJECTILE:
		if ( ent->e_ops->e_collide_world &&
			map_collide_line(map, ent->e_oldorigin,
				ent->e_origin, hit) ) {
			(*ent->e_ops->e_collide_world)(ent, hit);
		}
		break;
	case ENT_HELI:
		break;
	default:
		break;
	}
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
	vec3_t a;

	if ( NULL == ent->e_ops->e_render )
		return;

	v_copy(ent->e_oldlerp, ent->e_lerp);
	ent->e_lerp[0] = ent->e_oldorigin[0] + ent->e_move[0] * lerp;
	ent->e_lerp[1] = ent->e_oldorigin[1] + ent->e_move[1] * lerp;
	ent->e_lerp[2] = ent->e_oldorigin[2] + ent->e_move[2] * lerp;

	v_sub(a, ent->e_angles, ent->e_oldangles);
	v_scale(a, lerp);
	v_add(a, ent->e_oldangles);

	glPushMatrix();
	renderer_translate(r, ent->e_lerp[0], ent->e_lerp[1], ent->e_lerp[2]);
	renderer_rotate(r, a[2] * (180.0 / M_PI), 0, 1, 0);
	renderer_rotate(r, a[0] * (180.0 / M_PI), 1, 0, 0);
	renderer_rotate(r, a[1] * (180.0 / M_PI), 0, 0, 1);
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
