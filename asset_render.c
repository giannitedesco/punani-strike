/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/vec.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/asset.h>
#include <punani/blob.h>
#include <math.h>

#include "list.h"
#include "assetfile.h"

#include <punani/punani_gl.h>
#include <SDL.h>
#include <math.h>

#define M_INFINITY 200.0f

static void translate_light_pos(renderer_t r, vec3_t light_pos)
{
	vec3_t res;
	renderer_xlat_world_to_obj(r, res, light_pos);
	v_normalize(res);
	light_pos[0] = -res[0];
	light_pos[1] = -res[1];
	light_pos[2] = -res[2];
}

static void extrude_vert(struct _asset_file *f, unsigned int i)
{
	float *e = f->f_verts_ex;
	const float *v = f->f_verts;

	e[i * 3 + 0] = v[i * 3 + 0] + f->f_lightpos[0] * M_INFINITY;
	e[i * 3 + 1] = v[i * 3 + 1] + f->f_lightpos[1] * M_INFINITY;
	e[i * 3 + 2] = v[i * 3 + 2] + f->f_lightpos[2] * M_INFINITY;
}

static void extrude_verts(struct _asset_file *f)
{
	unsigned int i;

	for(i = 0; i < f->f_hdr->h_verts; i++) {
		extrude_vert(f, i);
	}
}

static void recalc_shadows(struct _asset_file *f, renderer_t r, light_t l)
{
	light_get_pos(l, f->f_lightpos);
	translate_light_pos(r, f->f_lightpos);
	extrude_verts(f);
}

void asset_file_render_begin(asset_file_t f, renderer_t r, light_t l)
{
	if ( l ) {
		recalc_shadows(f, r, l);
		f->f_shadows_dirty = 0;
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, f->f_verts);
	glNormalPointer(GL_FLOAT, 0, f->f_norms);
}

void asset_file_render_end(asset_file_t f)
{
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

static void render_vol(const float *s, const float *e, const float *n,
			uint16_t tri[3], const vec3_t light_pos)
{
	unsigned int i;
	vec3_t surf[3];
	vec3_t esurf[3];
	vec3_t a, b, c, d;

	for(i = 0; i < 3; i++) {
		surf[i][0] = s[tri[i] + 0];
		surf[i][1] = s[tri[i] + 1];
		surf[i][2] = s[tri[i] + 2];
	}

	for(i = 0; i < 3; i++) {
		esurf[i][0] = e[tri[i] + 0];
		esurf[i][1] = e[tri[i] + 1];
		esurf[i][2] = e[tri[i] + 2];
	}

	/* don't cast shadows for triangles not facing light */
	v_sub(a, surf[1], surf[0]);
	v_sub(b, surf[2], surf[0]);
	v_cross_product(c, a, b);
	v_normalize(c);
	for(i = 0; i < 3; i++)
		d[i] = light_pos[i];
	v_normalize(d);
	if ( v_dot_product(c, d) < 0 )
		return;

	glBegin(GL_TRIANGLES);
#if 1
	glVertex3fv(esurf[2]);
	glVertex3fv(esurf[1]);
	glVertex3fv(esurf[0]);

	glVertex3fv(surf[0]);
	glVertex3fv(surf[1]);
	glVertex3fv(surf[2]);
#endif

	for(i = 0; i < 3; i++) {
		unsigned int a, b;

		a = (i % 3);
		b = ((i + 1) % 3);

		glVertex3fv(surf[a]);
		glVertex3fv(esurf[a]);
		glVertex3fv(surf[b]);

		glVertex3fv(surf[b]);
		glVertex3fv(esurf[a]);
		glVertex3fv(esurf[b]);
	}
	glEnd();
}

static void render_shadow(asset_t a, renderer_t r, light_t l)
{
	const struct asset_desc *d = a->a_owner->f_desc + a->a_idx;
	unsigned int i;
	const float *norms;
	const float *verts;
	const float *evert;
	vec3_t light_pos;
	uint16_t tri[3];

	evert = a->a_owner->f_verts_ex;
	verts = a->a_owner->f_verts;
	norms = a->a_owner->f_norms;

	light_get_pos(l, light_pos);
	translate_light_pos(r, light_pos);

	for(i = 0; i < d->a_num_idx; i += 3) {
		tri[0] = a->a_indices[i + 0] * 3;
		tri[1] = a->a_indices[i + 1] * 3;
		tri[2] = a->a_indices[i + 2] * 3;

#if 1
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		if ( GLEW_EXT_stencil_two_side ) {
			glDisable(GL_CULL_FACE);
		}else{
			glEnable(GL_CULL_FACE);
		}
		glEnable(GL_STENCIL_TEST);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0f, 100.0f);

		if ( GLEW_EXT_stencil_two_side ) {
			glActiveStencilFaceEXT(GL_BACK);
			glStencilFunc(GL_ALWAYS, 0, ~0);
			glStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
		}else{
			glCullFace(GL_BACK);
			glStencilFunc(GL_ALWAYS, 0x0, 0xff);
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
			render_vol(verts, evert, norms, tri, light_pos);
		}

		if ( GLEW_EXT_stencil_two_side ) {
			glActiveStencilFaceEXT(GL_FRONT);
			glStencilFunc(GL_ALWAYS, 0, ~0);
			glStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
			glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		}else{
			glCullFace(GL_FRONT);
			glStencilFunc(GL_ALWAYS, 0x0, 0xff);
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
		}
#endif

		render_vol(verts, evert, norms, tri, light_pos);

		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_CULL_FACE);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}
}

static void render_asset(asset_t a, renderer_t r)
{
	const struct asset_desc *d = a->a_owner->f_desc + a->a_idx;
	glDrawElements(GL_TRIANGLES, d->a_num_idx,
			GL_UNSIGNED_SHORT, a->a_indices);
}

void asset_render(asset_t a, renderer_t r, light_t l)
{
	if ( l ) {
		render_shadow(a, r, l);
	}else{
		render_asset(a, r);
	}
}
