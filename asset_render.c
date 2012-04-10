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

#include <GL/gl.h>
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
static void render_tri(const fp_t *v, const float *n, uint16_t tri[3], vec3_t light_pos)
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
static float dot_product(vec3_t v1, vec3_t v2)
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

#define M_INFINITY 1000.0f
static void render_tri(const fp_t *s, const float *n, uint16_t tri[3], vec3_t light_pos)
{
	unsigned int i;
	vec3_t surf[3];
	float v[3][3];

	for(i = 0; i < 3; i++) {
		surf[i][0] = s[tri[i] + 0];
		surf[i][1] = s[tri[i] + 1];
		surf[i][2] = s[tri[i] + 2];
	}

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

	glBegin(GL_QUAD_STRIP);
	for(i = 0; i < 4; i++) {
		glVertex3fv(surf[i % 3]);
		glVertex3fv(v[i % 3]);
	}
	glEnd();
}
#endif

static void draw_shadow(void)
{
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	glDisable(GL_DEPTH_TEST);

	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(0, 1);
		glVertex2i(1, 1);
		glVertex2i(1, 0);
	glEnd();

	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

static void render_shadow(asset_t a, renderer_t r, light_t l)
{
	const struct asset_desc *d = a->a_owner->f_desc + a->a_idx;
	unsigned int i;
	const float *norms;
	const fp_t *verts;
	uint16_t tri[3];
	vec3_t light_pos;

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
#if 1
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		glEnable(GL_CULL_FACE);
		glEnable(GL_STENCIL_TEST);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0f, 100.0f);

		glCullFace(GL_FRONT);
		glStencilFunc(GL_ALWAYS, 0x0, 0xff);
		glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
		render_tri(verts, norms, tri, light_pos);

		glCullFace(GL_BACK);
		glStencilFunc(GL_ALWAYS, 0x0, 0xff);
		glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
#endif
		render_tri(verts, norms, tri, light_pos);

#if 1
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_CULL_FACE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);

		glStencilFunc(GL_NOTEQUAL, 0x0, 0xff);
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
		draw_shadow();
		glDisable(GL_STENCIL_TEST);
#endif
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
