/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/light.h>
#include <punani/world.h>
#include <punani/map.h>
#include <punani/chopper.h>

#include "game-modes.h"

#include <SDL/SDL_keysym.h>
#include <math.h>
#include <GL/gl.h>

struct _world {
	renderer_t render;
	map_t map;
	chopper_t apache;
	light_t light;
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

static void render(void *priv, float lerp)
{
	struct _world *world = priv;
	renderer_t r = world->render;
	vec3_t lpos;

again:
	lpos[0] = 0.0;
	lpos[1] = sin(world->lightAngle);
	lpos[2] = cos(world->lightAngle);
	if ( lpos[1] < 0.0 ) {
		world->lightAngle = 0.0;
		goto again;
	}

	renderer_render_3d(r);
	renderer_clear_color(r, 0.8, 0.8, 1.0);
	light_set_pos(world->light, lpos);

	/* look down on things */
	renderer_rotate(r, 65.0f, 1, 0, 0);
	renderer_rotate(r, 45.0f, 0, 1, 0);
	renderer_translate(r, 0.0, -50, 0.0);
	light_render(world->light);

	if ( world->do_shadows && lpos[1] >= 0.0f) {
		glDisable(GL_LIGHTING);
	}
	map_render(world->map, r, NULL);
	//chopper_render(world->apache, r, lerp, NULL);
	if ( world->do_shadows && lpos[1] >= 0.0f) {
		map_render(world->map, r, world->light);
		//chopper_render(world->apache, r, lerp, world->light);
	}else{
		chopper_render(world->apache, r, lerp, NULL);
		return;
	}
#if 1
	glEnable(GL_LIGHTING);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glStencilFunc(GL_EQUAL, 0x0, 0xff);
	//glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glEnable(GL_STENCIL_TEST);
	map_render(world->map, r, NULL);
	glDisable(GL_STENCIL_TEST);
#else
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glStencilFunc(GL_NOTEQUAL, 0x0, 0xff);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glEnable(GL_STENCIL_TEST);

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
	glDisable(GL_STENCIL_TEST);
#endif
	chopper_render(world->apache, r, lerp, NULL);
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
	chopper_think(world->apache);
}

const struct game_ops world_ops = {
	.ctor = ctor,
	.dtor = dtor,
	.new_frame = frame,
	.render = render,
	.keypress = keypress,
};
