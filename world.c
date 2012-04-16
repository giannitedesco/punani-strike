/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/vec.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/world.h>
#include <punani/map.h>
#include <punani/chopper.h>

#include "game-modes.h"

#include <SDL/SDL_keysym.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define CAMERA_HEIGHT	50.0
#define CHOPPER_HEIGHT	20.0

struct _world {
	renderer_t render;
	map_t map;
	chopper_t apache;
	light_t light;
	vec3_t lpos;
	vec3_t cpos;
	float lightAngle;
	int do_shadows;
};

static void *ctor(renderer_t r, void *common)
{
	struct _world *world = NULL;

	world = calloc(1, sizeof(*world));
	if ( NULL == world )
		goto out;

	world->render = r;

	world->map = map_load(r, "data/map/1.psm");
	if ( NULL == world->map )
		goto out_free;

	world->apache = chopper_comanche(-45.0, 50.0, 0.785);
	if ( NULL == world->apache )
		goto out_free_map;

	world->light = light_new(r);
	if ( NULL == world->light ) {
		goto out_free_chopper;
	}

	/* success */
	goto out;

out_free_chopper:
	chopper_free(world->apache);
out_free_map:
	map_free(world->map);
out_free:
	free(world);
	world = NULL;
out:
	return world;
}

static void calc_cpos(vec3_t cpos)
{
	double mvmatrix[16];
	double projmatrix[16];
	int viewport[4];
	double x, y;
	double near[3], far[3];
	double t;
	vec3_t a, b, v;

	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);

	x = viewport[2] / 2.0;
	y = viewport[3] / 2.0;
	gluUnProject(x, y, 0.0, mvmatrix, projmatrix, viewport,
			&near[0], &near[1], &near[2]);
	gluUnProject(x, y, 1.0, mvmatrix, projmatrix, viewport,
			&far[0], &far[1], &far[2]);

	a[0] = near[0];
	a[1] = near[1];
	a[2] = near[2];
	b[0] = far[0];
	b[1] = far[1];
	b[2] = far[2];
	v_sub(v, a, b);
	v_normalize(v);
	t = (CAMERA_HEIGHT - CHOPPER_HEIGHT) / v[1];
	v_scale(v, t);

	//printf("%f %f %f\n", v[0], v[1], v[2]);
	cpos[0] = -v[0];
	cpos[1] = v[1];
	cpos[2] = -v[2];
}

static void view_transform(world_t w)
{
	renderer_t r = w->render;
	renderer_rotate(r, 45.0f, 1, 0, 0);
	renderer_rotate(r, 45.0f, 0, 1, 0);
	renderer_translate(r, 0.0, -CAMERA_HEIGHT, 0.0);
	calc_cpos(w->cpos);
}

static void do_render(world_t w, float lerp, light_t l)
{
	renderer_t r = w->render;
	float x, y;

	chopper_get_pos(w->apache, &x, &y);
	glPushMatrix();
	renderer_translate(r, x, 0.0, y);
	map_render(w->map, r, l);
	glPopMatrix();

	glPushMatrix();
	renderer_translate(r, w->cpos[0], CHOPPER_HEIGHT, w->cpos[2]);
	chopper_render(w->apache, r, lerp, l);
	glPopMatrix();
}

static void render_lit(world_t w, float lerp)
{
	glEnable(GL_LIGHTING);

	if ( w->do_shadows ) {
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
		glStencilFunc(GL_EQUAL, 0x0, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glEnable(GL_STENCIL_TEST);
	}

	do_render(w, lerp, NULL);

	if ( w->do_shadows ) {
		glDisable(GL_STENCIL_TEST);
	}
}

static void render_shadow_volumes(world_t w, float lerp)
{
	if ( !w->do_shadows )
		return;
	do_render(w, lerp, w->light);
}

static void render_unlit(world_t w, float lerp)
{
	if ( !w->do_shadows )
		return;
	glDisable(GL_LIGHTING);
	do_render(w, lerp, NULL);
}

static void recalc_light(world_t w)
{
again:
	w->lpos[0] = 0.0;
	w->lpos[1] = sin(w->lightAngle);
	w->lpos[2] = cos(w->lightAngle);
	if ( w->lpos[1] < 0.0 ) {
		w->lightAngle = 0.0;
		goto again;
	}
}

static void render(void *priv, float lerp)
{
	struct _world *world = priv;
	renderer_t r = world->render;

	renderer_render_3d(r);
	renderer_clear_color(r, 0.8, 0.8, 1.0);
	light_set_pos(world->light, world->lpos);

	glPushMatrix();
	light_render(world->light);
	view_transform(world);

	render_unlit(world, lerp);
	render_shadow_volumes(world, lerp);
	render_lit(world, lerp);

	glPopMatrix();
}

static void dtor(void *priv)
{
	struct _world *world = priv;
	light_free(world->light);
	chopper_free(world->apache);
	map_free(world->map);
	free(world);
}

static void keypress(void *priv, int key, int down)
{
	struct _world *world = priv;
	switch(key) {
	case SDLK_LEFT:
		chopper_control(world->apache, CHOPPER_LEFT, down);
		break;
	case SDLK_RIGHT:
		chopper_control(world->apache, CHOPPER_RIGHT, down);
		break;
	case SDLK_UP:
		chopper_control(world->apache, CHOPPER_THROTTLE, down);
		break;
	case SDLK_DOWN:
		chopper_control(world->apache, CHOPPER_BRAKE, down);
		break;
	case SDLK_q:
	case SDLK_ESCAPE:
		renderer_exit(world->render, GAME_MODE_COMPLETE);
		break;
	case SDLK_SPACE:
		if ( down )
			world->do_shadows = !world->do_shadows;
		break;
	default:
		break;
	}
}

static void frame(void *priv)
{
	struct _world *world = priv;
	world->lightAngle += M_PI / 360.0;
	recalc_light(world);
	chopper_think(world->apache);
}

const struct game_ops world_ops = {
	.ctor = ctor,
	.dtor = dtor,
	.new_frame = frame,
	.render = render,
	.keypress = keypress,
};
