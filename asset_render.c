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

#define M_INFINITY 100.0f

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
	float *e = f->f_verts_ex + 3 * f->f_hdr->h_verts;
	const struct asset_vbo *v = f->f_verts;

	e[i * 3 + 0] = v[i].v_vert[0] + f->f_lightpos[0] * M_INFINITY;
	e[i * 3 + 1] = v[i].v_vert[1] + f->f_lightpos[1] * M_INFINITY;
	e[i * 3 + 2] = v[i].v_vert[2] + f->f_lightpos[2] * M_INFINITY;
}

static void copy_vert(struct _asset_file *f, unsigned int i)
{
	float *e = f->f_verts_ex;
	const struct asset_vbo *v = f->f_verts;

	e[i * 3 + 0] = v[i].v_vert[0];
	e[i * 3 + 1] = v[i].v_vert[1];
	e[i * 3 + 2] = v[i].v_vert[2];
}

static void extrude_verts(struct _asset_file *f)
{
	unsigned int i;

	for(i = 0; i < f->f_hdr->h_verts; i++) {
		copy_vert(f, i);
		extrude_vert(f, i);
	}
}

static void emit_vert(idx_t **out, idx_t a)
{
	**out = a;
	(*out)++;
}

static void emit_tri(idx_t **out, idx_t a, idx_t b, idx_t c)
{
	emit_vert(out, a);
	emit_vert(out, b);
	emit_vert(out, c);
}

static unsigned int calc_vol(struct _asset_file *f, uint16_t tri[3], idx_t *out)
{
	unsigned int i;
	vec3_t s[3];
	idx_t surf[3];
	idx_t esurf[3];
	vec3_t a, b, c, d;

	for(i = 0; i < 3; i++) {
		surf[i] = tri[i];
	}

	for(i = 0; i < 3; i++) {
		s[i][0] = f->f_verts[tri[i]].v_vert[0];
		s[i][1] = f->f_verts[tri[i]].v_vert[1];
		s[i][2] = f->f_verts[tri[i]].v_vert[2];
	}

	for(i = 0; i < 3; i++) {
		esurf[i] = tri[i] + f->f_hdr->h_verts;
	}

	/* don't cast shadows for triangles not facing light */
	v_sub(a, s[1], s[0]);
	v_sub(b, s[2], s[0]);
	v_cross_product(c, a, b);
	v_normalize(c);
	for(i = 0; i < 3; i++)
		d[i] = f->f_lightpos[i];
	v_normalize(d);
	if ( v_dot_product(c, d) < 0 )
		return 0;

	emit_tri(&out, esurf[2], esurf[1], esurf[0]);
	emit_tri(&out, surf[0], surf[1], surf[2]);

	for(i = 0; i < 3; i++) {
		unsigned int a, b;

		a = (i % 3);
		b = ((i + 1) % 3);

		emit_tri(&out, surf[a], esurf[a], surf[b]);
		emit_tri(&out, surf[b], esurf[a], esurf[b]);
	}

	return 8;
}

static unsigned int calc_asset_vol(struct _asset *a)
{
	struct _asset_file *f = a->a_owner;
	const struct asset_desc *d = f->f_desc + a->a_idx;
	unsigned int i, num_tris, ret = 0;
	idx_t tri[3];
	idx_t *out = a->a_shadow_idx;

	for(i = 0; i < d->a_num_idx; i += 3) {
		tri[0] = a->a_indices[i + 0];
		tri[1] = a->a_indices[i + 1];
		tri[2] = a->a_indices[i + 2];
		num_tris = calc_vol(f, tri, out);
		out += num_tris * 3;
		ret += num_tris * 3;
	}

	return ret;
}

static void update_shadow_buffers(struct _asset_file *f)
{
	if ( !f->f_vbo_shadow ) {
		glGenBuffers(1, &f->f_vbo_shadow);
		printf("VBO: ok: %u\n", f->f_vbo_shadow);
	}

	if ( !f->f_ibo_shadow ) {
		glGenBuffers(1, &f->f_ibo_shadow);
		printf("IBO: ok: %u\n", f->f_ibo_shadow);
	}

	glBindBuffer(GL_ARRAY_BUFFER, f->f_vbo_shadow);
	glBufferData(GL_ARRAY_BUFFER,
			sizeof(*f->f_verts_ex) * f->f_hdr->h_verts * 6,
			NULL, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER,
			sizeof(*f->f_verts_ex) * f->f_hdr->h_verts * 6,
			f->f_verts_ex, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, f->f_ibo_shadow);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			sizeof(*f->f_idx_shadow) * f->f_shadow_indices,
			NULL, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			sizeof(*f->f_idx_shadow) * f->f_shadow_indices,
			f->f_idx_shadow, GL_STATIC_DRAW);
}

static void recalc_shadows(struct _asset_file *f, renderer_t r, light_t l)
{
	unsigned int i, shadow_off = 0;

	/* get light position */
	light_get_pos(l, f->f_lightpos);
	translate_light_pos(r, f->f_lightpos);

	/* extrude all vertices */
	extrude_verts(f);

	/* construct shadow geometry for all loaded assets */
	for(i = 0; i < f->f_hdr->h_num_assets; i++) {
		struct _asset *a;

		a = f->f_db[i];
		if ( NULL == a )
			continue;

		a->a_shadow_idx = f->f_idx_shadow + shadow_off;
		a->a_num_shadow_idx = calc_asset_vol(a);
		shadow_off += a->a_num_shadow_idx;
	}

	update_shadow_buffers(f);
}

void asset_file_render_begin(asset_file_t f, renderer_t r, light_t l)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	if ( l && f->f_shadows_dirty ) {
		recalc_shadows(f, r, l);
		f->f_shadows_dirty = 0;
	}

	if ( !f->f_vbo_geom ) {
		glGenBuffers(1, &f->f_vbo_geom);
		printf("VBO: ok: %u\n", f->f_vbo_geom);
		glBindBuffer(GL_ARRAY_BUFFER, f->f_vbo_geom);
		glBufferData(GL_ARRAY_BUFFER,
				sizeof(*f->f_verts) * f->f_hdr->h_verts * 2,
				NULL, GL_STATIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER,
				sizeof(*f->f_verts) * f->f_hdr->h_verts,
				f->f_verts, GL_STATIC_DRAW);
	}

	if ( !f->f_ibo_geom ) {
		glGenBuffers(1, &f->f_ibo_geom);
		printf("IBO: ok: %u\n", f->f_ibo_geom);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, f->f_ibo_geom);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(*f->f_idx_begin) * f->f_num_indices,
				NULL, GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(*f->f_idx_begin) * f->f_num_indices,
				f->f_idx_begin, GL_STATIC_DRAW);
	}
}

void asset_file_render_end(asset_file_t f)
{
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

static void render_vol(struct _asset *a)
{
	struct _asset_file *f = a->a_owner;

	glBindBuffer(GL_ARRAY_BUFFER, f->f_vbo_shadow);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, f->f_ibo_shadow);

	glVertexPointer(3, GL_FLOAT, 0, 0);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDrawElements(GL_TRIANGLES, a->a_num_shadow_idx,
			GL_UNSIGNED_SHORT,
			(void *)((a->a_shadow_idx -
				f->f_idx_shadow) * sizeof(idx_t)));
	glEnableClientState(GL_NORMAL_ARRAY);
}

static void render_shadow(asset_t a, renderer_t r, light_t l)
{
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
		render_vol(a);
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

	render_vol(a);

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

static void render_asset(asset_t a, renderer_t r)
{
	struct _asset_file *f = a->a_owner;
	const struct asset_desc *d = f->f_desc + a->a_idx;

	glBindBuffer(GL_ARRAY_BUFFER, f->f_vbo_geom);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, f->f_ibo_geom);

	glVertexPointer(3, GL_FLOAT, sizeof(*f->f_verts), 0);
	glNormalPointer(GL_FLOAT, sizeof(*f->f_verts), (void *)12);

	glDrawElements(GL_TRIANGLES, d->a_num_idx,
			GL_UNSIGNED_SHORT,
			(void *)((a->a_indices - f->f_idx_begin) * sizeof(idx_t)));
}

void asset_render(asset_t a, renderer_t r, light_t l)
{
	if ( l ) {
		render_shadow(a, r, l);
	}else{
		render_asset(a, r);
	}
}
