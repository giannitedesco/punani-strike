/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/vec.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/asset.h>
#include <punani/missile.h>
#include <punani/particles.h>
#include <punani/map.h>
#include <punani/entity.h>
#include <punani/cvar.h>
#include <math.h>
#include "ent-internal.h"

struct _missile {
	struct _entity m_ent;
	asset_file_t m_assets;
	asset_t m_mesh;
	particles_t m_trail;
	float m_velocity;
	float m_heading;
	unsigned int m_lifetime;
};

static void render(struct _entity *ent, renderer_t r, float lerp, light_t l)
{
	struct _missile *m = (struct _missile *)ent;

	renderer_rotate(r, m->m_heading * (180.0 / M_PI), 0, 1, 0);
	//asset_file_dirty_shadows(c->asset);
	//asset_file_render_begin(c->asset, r, l);
	//asset_render(m->m_mesh, r, l);
	//asset_file_render_end(c->asset);

	particles_emit(m->m_trail, m->m_ent.e_oldlerp, m->m_ent.e_lerp);
}

static void think(struct _entity *ent)
{
	struct _missile *m = (struct _missile *)ent;

	m->m_lifetime--;
	if ( !m->m_lifetime || m->m_ent.e_origin[1] <= 0.0 ) {
		entity_unlink(&m->m_ent);
		return;
	}

	v_copy(m->m_ent.e_oldorigin, m->m_ent.e_origin);
	v_add(m->m_ent.e_origin, m->m_ent.e_move);
//	printf("missile %f %f %f\n", m->m_ent.e_origin[0], m->m_ent.e_origin[1], m->m_ent.e_origin[2]);
}

static void dtor(struct _entity *ent)
{
	struct _missile *m = (struct _missile *)ent;

	asset_put(m->m_mesh);
	asset_file_close(m->m_assets);
	particles_unref(m->m_trail);
	free(m);
}

static void collide_world(struct _entity *ent, const vec3_t hit)
{
	struct _missile *m = (struct _missile *)ent;
	entity_unlink(&m->m_ent);
}

static const struct entity_ops ops = {
	.e_flags = ENT_PROJECTILE,
	.e_render = render,
	.e_think = think,
	.e_collide_world = collide_world,
	.e_dtor = dtor,
};

missile_t missile_spawn(const vec3_t origin, float heading, float pitch)
{
	struct _missile *m;

	m = calloc(1, sizeof(*m));
	if ( NULL == m )
		return NULL;

	m->m_assets = asset_file_open("data/missiles.db");
	if ( NULL == m->m_assets )
		goto err_free;

	m->m_mesh = asset_file_get(m->m_assets, "AGR_71_Hydra.g");
	if ( NULL == m->m_mesh )
		goto err_free_assets;

	m->m_trail = particles_new(1024);
	if ( NULL == m->m_trail )
		goto err_free_mesh;

	m->m_lifetime = 100;
	m->m_velocity = 12;
	m->m_heading = heading;
	entity_spawn(&m->m_ent, &ops, origin, NULL);
	m->m_ent.e_move[0] += sin(m->m_heading);
	m->m_ent.e_move[1] = cos(pitch);
	m->m_ent.e_move[2] += cos(m->m_heading);
	v_normalize(m->m_ent.e_move);
	v_scale(m->m_ent.e_move, m->m_velocity);
	entity_link(&m->m_ent);

	/* success */
	//con_printf("Missile away: %f %f %f\n",
	//	m->m_ent.e_origin[0],
	//	m->m_ent.e_origin[1],
	//	m->m_ent.e_origin[2]);
	goto out;

err_free_mesh:
	asset_put(m->m_mesh);
err_free_assets:
	asset_file_close(m->m_assets);
err_free:
	free(m);
	m = NULL;
out:
	return m;
}
