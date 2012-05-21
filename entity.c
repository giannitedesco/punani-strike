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

static void collide_projectile(struct _entity *ent, map_t map)
{
	vec3_t hit;

	if ( ent->e_ops->e_collide_world &&
		map_collide_line(map, ent->e_oldorigin,
			ent->e_origin, hit) ) {
		(*ent->e_ops->e_collide_world)(ent, hit);
	}
}

struct shim {
	struct _entity *ent;
	asset_t mesh;
	int coarse, fine;
};

static int cb(const struct map_hit *hit, void *priv)
{
	struct shim *shim = priv;
	shim->coarse++;

#if 0
	float n, delta;
	delta = (hit->times[1] - hit->times[2]) / 100.0;
	for(n = hit->times[0]; n <= hit->times[1]; n += delta) {
		vec3_t pos;
		vec3_t angles;

		pos[0] = shim->ent->e_oldorigin[0] + shim->ent->e_move[0] * n;
		pos[1] = shim->ent->e_oldorigin[1] + shim->ent->e_move[1] * n;
		pos[2] = shim->ent->e_oldorigin[2] + shim->ent->e_move[2] * n;

		v_sub(angles, shim->ent->e_angles, shim->ent->e_oldangles);
		v_scale(angles, n);
		v_add(angles, angles, shim->ent->e_oldangles);
	}
#endif

	return 1;
}

static void collide_heli(struct _entity *ent, map_t map)
{
	unsigned int i, num_mesh;
	struct shim shim;
	vec3_t mins, maxs;

	num_mesh = (*ent->e_ops->e_num_meshes)(ent);
	shim.ent = ent;
	ent->collide = 0;
	for(i = 0; i < num_mesh; i++) {
		struct obb obb;
		asset_t a;

		a = (*ent->e_ops->e_mesh)(ent, i);
		shim.mesh = a;
		shim.coarse = 0;
		shim.fine = 0;

		asset_mins(shim.mesh, mins);
		asset_maxs(shim.mesh, maxs);
		obb_from_aabb(&obb, mins, maxs);
#if 1
		basis_rotateY(obb.rot, ent->e_angles[1]);
		basis_rotateX(obb.rot, ent->e_angles[0]);
		basis_rotateZ(obb.rot, ent->e_angles[2]);
#endif
		//v_add(obb.origin, obb.origin, ent->e_origin);
		v_copy(obb.origin, ent->e_oldorigin);
		v_copy(obb.vel, ent->e_move);

		map_sweep(map, &obb, cb, &shim);
		if ( shim.coarse ) {
			(*ent->e_ops->e_collide_world)(ent, NULL);
			ent->collide = 1;
		}
	}
}

static void entity_think(struct _entity *ent, map_t map)
{
	v_copy(ent->e_oldorigin, ent->e_origin);
	v_copy(ent->e_oldangles, ent->e_angles);

	if ( ent->e_ops->e_think )
		(*ent->e_ops->e_think)(ent);

	v_add(ent->e_origin, ent->e_origin, ent->e_move);

	switch(ent->e_ops->e_flags & ENT_TYPE_MASK) {
	case ENT_PROJECTILE:
		collide_projectile(ent, map);
		break;
	case ENT_HELI:
		collide_heli(ent, map);
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

static void obb_vert(struct obb *obb, float x, float y, float z)
{
	vec3_t vec, tmp = {x, y, z};
	basis_transform((const float (*)[3])obb->rot, vec, tmp);
	glVertex3f(vec[0], vec[1], vec[2]);
}

static void draw_obb(struct _entity *ent, renderer_t r, vec3_t angles)
{
	unsigned int i, num_mesh;
	vec3_t mins, maxs;
	struct obb obb;

	renderer_wireframe(r, 1);
	glEnable(GL_DEPTH_TEST);

	num_mesh = (*ent->e_ops->e_num_meshes)(ent);
	for(i = 0; i < num_mesh; i++) {
		asset_t a;

		a = (*ent->e_ops->e_mesh)(ent, i);
		asset_mins(a, mins);
		asset_maxs(a, maxs);
		obb_from_aabb(&obb, mins, maxs);

#if 1
		basis_rotateZ(obb.rot, -angles[2]);
		basis_rotateX(obb.rot, -angles[0]);
		basis_rotateY(obb.rot, angles[1]);
#endif
		//obb_build_aabb(&obb, mins, maxs);

		if ( ent->collide ) {
			glColor4f(1.0, 0.0, 0.0, 1.0);
		}else{
			glColor4f(0.0, 1.0, 0.0, 1.0);
			continue;
		}
		glBegin(GL_QUADS);
		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] - obb.dim[2]);
		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] - obb.dim[2]);
		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] - obb.dim[2]);
		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] - obb.dim[2]);

		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] - obb.dim[2]);
		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] - obb.dim[2]);

		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] + obb.dim[2]);

		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] - obb.dim[2]);
		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] - obb.dim[2]);

		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] - obb.dim[2]);
		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] - obb.dim[1],
				obb.origin[2] - obb.dim[2]);

		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] - obb.dim[2]);
		obb_vert(&obb, obb.origin[0] - obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] + obb.dim[2]);
		obb_vert(&obb, obb.origin[0] + obb.dim[0],
				obb.origin[1] + obb.dim[1],
				obb.origin[2] - obb.dim[2]);

#if 0
		glColor4f(0.0, 1.0, 0.0, 1.0);
		glVertex3f(mins[0], mins[1], mins[2]);
		glVertex3f(maxs[0], mins[1], mins[2]);
		glVertex3f(maxs[0], maxs[1], mins[2]);
		glVertex3f(mins[0], maxs[1], mins[2]);

		glVertex3f(mins[0], mins[1], mins[2]);
		glVertex3f(mins[0], mins[1], maxs[2]);
		glVertex3f(mins[0], maxs[1], maxs[2]);
		glVertex3f(mins[0], maxs[1], mins[2]);

		glVertex3f(mins[0], mins[1], maxs[2]);
		glVertex3f(maxs[0], mins[1], maxs[2]);
		glVertex3f(maxs[0], maxs[1], maxs[2]);
		glVertex3f(mins[0], maxs[1], maxs[2]);

		glVertex3f(maxs[0], mins[1], mins[2]);
		glVertex3f(maxs[0], mins[1], maxs[2]);
		glVertex3f(maxs[0], maxs[1], maxs[2]);
		glVertex3f(maxs[0], maxs[1], mins[2]);
#endif
		glEnd();
	}

	renderer_wireframe(r, 0);
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
	v_add(a, a, ent->e_oldangles);

	glPushMatrix();
	renderer_translate(r, ent->e_lerp[0], ent->e_lerp[1], ent->e_lerp[2]);
	renderer_rotate(r, a[1] * (180.0 / M_PI), 0, 1, 0);
	renderer_rotate(r, a[0] * (180.0 / M_PI), 1, 0, 0);
	renderer_rotate(r, a[2] * (180.0 / M_PI), 0, 0, 1);
	(*ent->e_ops->e_render)(ent, r, lerp, l);
	glPopMatrix();

	glPushMatrix();
	if ( (ent->e_ops->e_flags & ENT_TYPE_MASK) == ENT_HELI ) {
		renderer_translate(r, ent->e_lerp[0], ent->e_lerp[1], ent->e_lerp[2]);
		draw_obb(ent, r, a);
	}
	glPopMatrix();
}

void entity_render_all(renderer_t r, float lerp, light_t l)
{
	entity_t ent, tmp;

	list_for_each_entry_safe(ent, tmp, &ents, e_list) {
		entity_render(ent, r, lerp, l);
	}
}
