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

#include "assetfile.h"

#include <punani/punani_gl.h>
#include <SDL.h>
#include <math.h>

/* Build the VBOs */
void asset_file_render_prepare(asset_file_t f) {
	if (f->f_vbo_verts == 0) {
		glGenBuffers(1, &f->f_vbo_verts);
		if (glGetError() == GL_NO_ERROR) {
			glBindBuffer(GL_ARRAY_BUFFER, f->f_vbo_verts);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * f->f_hdr->h_verts * 3, f->f_verts, GL_STATIC_DRAW);
		} else {
			f->f_vbo_verts = -1;
		}
	}
	if (f->f_vbo_norms == 0) {
		glGenBuffers(1, &f->f_vbo_norms);
		if (glGetError() == GL_NO_ERROR) {
			glBindBuffer(GL_ARRAY_BUFFER, f->f_vbo_norms);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * f->f_hdr->h_verts * 3, f->f_norms, GL_STATIC_DRAW);
		} else {
			f->f_vbo_verts = -1;
		}
	}
}

void asset_file_render_begin(asset_file_t f)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
#if ASSET_USE_FLOAT
	if (f->f_vbo_verts > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, f->f_vbo_verts);
		glVertexPointer(3, GL_FLOAT, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	} else {
		glVertexPointer(3, GL_FLOAT, 0, f->f_verts);
	}
#else
	glVertexPointer(3, GL_SHORT, 0, f->f_verts);
#endif

	if (f->f_vbo_norms > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, f->f_vbo_norms);
		glNormalPointer(GL_FLOAT, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	} else {
		glNormalPointer(GL_FLOAT, 0, f->f_norms);
	}
}

void asset_file_render_end(asset_file_t f)
{
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

#if 0
static void render_vol(const fp_t *v, const float *n, uint16_t tri[3], vec3_t light_pos)
{
	unsigned int i;
	glBegin(GL_TRIANGLES);
	for(i = 0; i < 3; i++) {
		glNormal3f(n[tri[i] + 0], n[tri[i] + 1], n[tri[i] + 2]);
		glVertex3f(v[tri[i] + 0], v[tri[i] + 1], v[tri[i] + 2]);
	}
	glEnd();
}
#else

#define M_INFINITY 200.0f
static void render_vol(const fp_t *s, const float *n,
			uint16_t tri[3], const vec3_t light_pos)
{
	unsigned int i;
	vec3_t surf[3];
	float v[3][3];
	vec3_t a, b, c, d;

	for(i = 0; i < 3; i++) {
		surf[i][0] = s[tri[i] + 0];
		surf[i][1] = s[tri[i] + 1];
		surf[i][2] = s[tri[i] + 2];
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

	for(i = 0; i < 3; i++) {
		v[i][0] = surf[i][0] + light_pos[0] * M_INFINITY;
		v[i][1] = surf[i][1] + light_pos[1] * M_INFINITY;
		v[i][2] = surf[i][2] + light_pos[2] * M_INFINITY;
	}

#if 1
	glBegin(GL_TRIANGLES);
	glVertex3fv(v[2]);
	glVertex3fv(v[1]);
	glVertex3fv(v[0]);
	glEnd();

	glBegin(GL_TRIANGLES);
	glVertex3fv(surf[0]);
	glVertex3fv(surf[1]);
	glVertex3fv(surf[2]);
	glEnd();
#endif

	glBegin(GL_QUAD_STRIP);
	for(i = 0; i < 4; i++) {
		glVertex3fv(surf[i % 3]);
		glVertex3fv(v[i % 3]);
	}
	glEnd();
}
#endif

static void translate_light_pos(renderer_t r, vec3_t light_pos)
{
	vec3_t res;
	renderer_xlat_world_to_obj(r, res, light_pos);
	v_normalize(res);
	light_pos[0] = -res[0];
	light_pos[1] = -res[1];
	light_pos[2] = -res[2];
}

static void render_shadow(asset_t a, renderer_t r, light_t l)
{
	const struct asset_desc *d = a->a_owner->f_desc + a->a_idx;
	unsigned int i;
	const float *norms;
	const fp_t *verts;
	vec3_t light_pos;
	uint16_t tri[3];

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
			render_vol(verts, norms, tri, light_pos);
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

		render_vol(verts, norms, tri, light_pos);

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
