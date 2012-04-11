/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/asset.h>
#include <punani/blob.h>
#include <math.h>

#include "assetfile.h"

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <math.h>

void asset_file_render_begin(asset_file_t f)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
#if ASSET_USE_FLOAT
	glVertexPointer(3, GL_FLOAT, 0, f->f_verts);
#else
	glVertexPointer(3, GL_SHORT, 0, f->f_verts);
#endif
	glNormalPointer(GL_FLOAT, 0, f->f_norms);
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
#define X 0
#define Y 1
#define Z 2
static float v_len(const vec3_t v)
{
	float len;

	len = (v[X] * v[X]) +
		(v[Y] * v[Y]) +
		(v[Z] * v[Z]);

	return sqrt(len);
}
static void v_normalize(vec3_t v)
{
	float len = v_len(v);

	if ( len == 0.0f )
		return;

	len = 1 / len;
	v[X] *= len;
	v[Y] *= len;
	v[Z] *= len;
}
static void v_sub(vec3_t d, const vec3_t v1, const vec3_t v2)
{
	d[X]= v1[X] - v2[X];
	d[Y]= v1[Y] - v2[Y];
	d[Z]= v1[Z] - v2[Z];
}
static void  cross_product(vec3_t d, const vec3_t v1, const vec3_t v2)
{
	d[X] = (v1[Y] * v2[Z]) - (v1[Z] * v2[Y]);
	d[Y] = (v1[Z] * v2[X]) - (v1[X] * v2[Z]);
	d[Z] = (v1[X] * v2[Y]) - (v1[Y] * v2[X]);
}
static float dot_product(const vec3_t v1, const vec3_t v2)
{
	return (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]);
}

static void normalize(vec3_t v)
{
	float f = 1.0f / sqrt(dot_product(v, v));

	v[0] *= f;
	v[1] *= f;
	v[2] *= f;
}

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
	cross_product(c, a, b);
	v_normalize(c);
	for(i = 0; i < 3; i++)
		d[i] = light_pos[i];
	v_normalize(d);
	if ( dot_product(c, d) < 0 )
		return;

	for(i = 0; i < 3; i++) {
		v[i][0] = surf[i][0] - light_pos[0];
		v[i][1] = surf[i][1] - light_pos[1];
		v[i][2] = surf[i][2] - light_pos[2];
		normalize(v[i]);
		v[i][0] *= M_INFINITY;
		v[i][1] *= M_INFINITY;
		v[i][2] *= M_INFINITY;
		v[i][0] += light_pos[0];
		v[i][1] += light_pos[1];
		v[i][2] += light_pos[2];
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

static void render_shadow(asset_t a, renderer_t r, light_t l)
{
	const struct asset_desc *d = a->a_owner->f_desc + a->a_idx;
	unsigned int i;
	const float *norms;
	const fp_t *verts;
	uint16_t tri[3];
	vec3_t light_pos;
	static PFNGLACTIVESTENCILFACEEXTPROC ffs_glActiveStencilFaceEXT;

	if ( NULL == ffs_glActiveStencilFaceEXT ) {
		ffs_glActiveStencilFaceEXT = SDL_GL_GetProcAddress("glActiveStencilFaceEXT");
	}

	verts = a->a_owner->f_verts;
	norms = a->a_owner->f_norms;

	light_get_pos(l, light_pos);
	light_pos[0] *= 100.0;
	light_pos[1] *= 100.0;
	light_pos[2] *= 100.0;

	for(i = 0; i < d->a_num_idx; i += 3) {
		tri[0] = a->a_indices[i + 0] * 3;
		tri[1] = a->a_indices[i + 1] * 3;
		tri[2] = a->a_indices[i + 2] * 3;

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		if ( ffs_glActiveStencilFaceEXT ) {
			glDisable(GL_CULL_FACE);
		}else{
			glEnable(GL_CULL_FACE);
		}
		glEnable(GL_STENCIL_TEST);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0f, 100.0f);

		if ( ffs_glActiveStencilFaceEXT ) {
			ffs_glActiveStencilFaceEXT(GL_BACK);
			glStencilFunc(GL_ALWAYS, 0, ~0);
			glStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
		}else{
			glCullFace(GL_BACK);
			glStencilFunc(GL_ALWAYS, 0x0, 0xff);
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
			render_vol(verts, norms, tri, light_pos);
		}

		if ( ffs_glActiveStencilFaceEXT ) {
			ffs_glActiveStencilFaceEXT(GL_FRONT);
			glStencilFunc(GL_ALWAYS, 0, ~0);
			glStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
			glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		}else{
			glCullFace(GL_FRONT);
			glStencilFunc(GL_ALWAYS, 0x0, 0xff);
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
		}

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
