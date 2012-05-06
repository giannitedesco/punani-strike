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
#include <punani/font.h>
#include <punani/chopper.h>
#include <punani/particles.h>

#include "game-modes.h"

#include <SDL/SDL_keysym.h>
#include <math.h>
#include <punani/punani_gl.h>

#define CAMERA_HEIGHT	120.0
#define CHOPPER_HEIGHT	55.0

struct _world {
	renderer_t render;
	map_t map;
	chopper_t apache;
	light_t light;
	font_t font;
	vec3_t lpos;
	vec3_t cpos;
	float lightAngle;
	int do_shadows;
	unsigned int fcnt;
};

static void *ctor(renderer_t r, void *common)
{
	struct _world *world = NULL;
	vec3_t spawn;

	world = calloc(1, sizeof(*world));
	if ( NULL == world )
		goto out;

	world->render = r;
	renderer_viewangles(r, 45.0, 45.0, 0.0);

	world->map = map_load(r, "data/maps/level1");
	if ( NULL == world->map )
		goto out_free;

	spawn[0] = 0.0;
	spawn[1] = CHOPPER_HEIGHT;
	spawn[2] = 0.0;

	world->apache = chopper_comanche(spawn, 0.785);
	if ( NULL == world->apache )
		goto out_free_map;

	world->light = light_new(r, LIGHT_CAST_SHADOWS);
	if ( NULL == world->light ) {
		goto out_free_chopper;
	}

	world->font = font_load(r, "data/font/carbon.png");
	if ( NULL == world->font )
		goto out_free_light;

	/* success */
	goto out;

out_free_light:
	light_free(world->light);
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

static void get_screen_centre(renderer_t r, float h, vec3_t out)
{
	unsigned int x, y;
	renderer_size(r, &x, &y);
	renderer_unproject(r, out, x / 2, y / 2, h);
}

static void view_transform(world_t w)
{
	renderer_t r = w->render;
	renderer_translate(r, 0.0, -CAMERA_HEIGHT, 0.0);
	get_screen_centre(r, CHOPPER_HEIGHT, w->cpos);
}

static void do_render(world_t w, float lerp, light_t l)
{
	renderer_t r = w->render;
	vec3_t cpos;

	chopper_get_pos(w->apache, lerp, cpos);

	glPushMatrix();
	renderer_translate(r, w->cpos[0], w->cpos[1], w->cpos[2]);
	renderer_translate(r, -cpos[0], -cpos[1], -cpos[2]);
	map_render(w->map, r, l);
	chopper_render_missiles(w->apache, r, lerp, l);
	glPopMatrix();

	glPushMatrix();
	renderer_translate(r, w->cpos[0], w->cpos[1], w->cpos[2]);
	chopper_render(w->apache, r, lerp, l);
	glPopMatrix();
}

static void render_lit(world_t w, float lerp)
{
	renderer_t r = w->render;
	vec3_t cpos;

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

	glPushMatrix();
	chopper_get_pos(w->apache, lerp, cpos);
	renderer_translate(r, w->cpos[0], w->cpos[1], w->cpos[2]);
	renderer_translate(r, -cpos[0], -cpos[1], -cpos[2]);
	particles_render_all(lerp);
	glPopMatrix();
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
	static const vec3_t c_noon = {1.0, 0.87, 1.0};
	static const vec3_t c_dusk = {1.0, 0.5, 0.3};
	static const vec3_t c_night = {0.1, 0.1, 0.4};
	vec3_t color;
	float lerp;

	w->lpos[0] = 0.0;
	w->lpos[1] = sin(w->lightAngle);
	w->lpos[2] = cos(w->lightAngle);
	if ( w->lpos[1] >= 0.0 ) {
		lerp = w->lpos[1];
		color[0] = c_dusk[0] + (c_noon[0] - c_dusk[0]) * lerp;
		color[1] = c_dusk[1] + (c_noon[1] - c_dusk[1]) * lerp;
		color[2] = c_dusk[2] + (c_noon[2] - c_dusk[2]) * lerp;
	}else{
		lerp = -(w->lpos[1] * 24);
		if ( lerp > 1.0 )
			lerp = 1.0;
		w->lpos[1] = sin(-w->lightAngle);
		w->lpos[2] = cos(-w->lightAngle);
		color[0] = c_dusk[0] + (c_night[0] - c_dusk[0]) * lerp;
		color[1] = c_dusk[1] + (c_night[1] - c_dusk[1]) * lerp;
		color[2] = c_dusk[2] + (c_night[2] - c_dusk[2]) * lerp;
	}

	light_set_color(w->light, color[0], color[1], color[2]);
	light_set_pos(w->light, w->lpos);
}

static void render(void *priv, float lerp)
{
	struct _world *world = priv;
	renderer_t r = world->render;
	vec3_t cpos;
	int mins;

	renderer_render_3d(r);
	renderer_clear_color(r, 0.8, 0.8, 1.0);

	glPushMatrix();
	view_transform(world);
	light_render(world->light);

	render_unlit(world, lerp);
	render_shadow_volumes(world, lerp);
	render_lit(world, lerp);

	glPopMatrix();

	renderer_render_2d(r);
	chopper_get_pos(world->apache, lerp, cpos);
	font_printf(world->font, 8, 4, "A madman strikes again! (%.0f fps)",
			renderer_fps(r));
	font_printf(world->font, 8, 24, "x: %.3f y: %.3f", cpos[0], cpos[2]);

	mins = (world->fcnt * (M_PI / 14400.0)) * (1440.0 / (2 * M_PI));
	mins += 6 * 60;
	mins %= 1440;
	font_printf(world->font, 8, 44, "local time: %02d:%02d",
			mins / 60, mins % 60);
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
			chopper_fire(world->apache, world->fcnt);
		break;
	case SDLK_1:
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

	if ( (world->fcnt % 10) == 0 ) {
		world->lightAngle += M_PI / 1440.0;
		recalc_light(world);
	}

	world->fcnt++;
	chopper_think(world->apache);
	particles_think_all();
}

const struct game_ops world_ops = {
	.ctor = ctor,
	.dtor = dtor,
	.new_frame = frame,
	.render = render,
	.keypress = keypress,
};
