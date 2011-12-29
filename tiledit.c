/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/game.h>
#include <punani/renderer.h>
#include <punani/map.h>
#include <punani/tex.h>

#include "render-internal.h"
#include "dessert-stroke.h"
#include "game-modes.h"

#include <SDL/SDL_keysym.h>

struct tiledit_common {
	const char *fn;
};

#define MOUSE_MAP_MOVE		1
#define MOUSE_TILE_DRAG		2

#define POS_MAP		0
#define POS_PALETTE	1

struct tiledit {
	renderer_t render;
	texture_t tiles;
	unsigned int tw, th;
	unsigned int mapw, maph;
	int mapx, mapy;
	unsigned int mouse_pos;
	unsigned int mouse_state;
	unsigned int lwidth;
	map_t map;
};

static void *ctor(renderer_t r, void *priv)
{	
	struct tiledit_common *common = priv;
	struct tiledit *tiledit = NULL;

	tiledit = calloc(1, sizeof(*tiledit));
	if ( NULL == tiledit )
		return NULL;

	tiledit->render = r;

	tiledit->map = map_load(common->fn);
	if ( NULL == tiledit->map )
		goto out_free;

	map_get_size(tiledit->map, &tiledit->mapw, &tiledit->maph);
	tiledit->tiles = map_get_tiles(tiledit->map,
					&tiledit->tw, &tiledit->th);

	tiledit->lwidth = texture_width(tiledit->tiles);

	/* success */
	goto out;

out_free:
	free(tiledit);
	tiledit = NULL;
out:
	return tiledit;
}

static void m_render(void *priv, texture_t tex, prect_t *src, prect_t *dst)
{
	struct tiledit *tiledit = priv;
	prect_t d, s;

	if ( dst ) {
		d.x = dst->x + tiledit->lwidth;
		d.y = dst->y;
		d.w = dst->w;
		d.h = dst->h;
	}else{
		d.x = tiledit->lwidth;
		d.y = 0;
	}

	s.x = src->x;
	s.y = src->y;
	s.w = src->w;
	s.h = src->h;

	if ( d.x < (int)tiledit->lwidth ) {
		int diff = tiledit->lwidth - d.x;
		s.x += diff;
		s.w -= diff;
		d.x += diff;
		d.w -= diff;
	}

	renderer_blit(tiledit->render, tex, &s, &d);
}

static void scroll_to(struct tiledit *tiledit,
			unsigned int x, unsigned int y)
{
	unsigned int sx, sy;
	renderer_size(tiledit->render, &sx, &sy);

	tiledit->mapx = r_clamp(x, 0, tiledit->mapw - (int)sx);
	tiledit->mapy = r_clamp(y, 0, tiledit->maph - (int)sy);
}
static const struct render_ops r_ops = {
	.blit = m_render,
};
static void render(void *priv, float lerp)
{
	struct tiledit *tiledit = priv;
	unsigned int sx, sy;
	struct _renderer r = {
		.ops = &r_ops,
		.priv = tiledit,
	};
	prect_t src;

	renderer_size(tiledit->render, &sx, &sy);

	src.x = tiledit->mapx;
	src.y = tiledit->mapy;
	src.w = sx - tiledit->lwidth;
	src.h = sy;

	renderer_blit(tiledit->render, tiledit->tiles, NULL, NULL);
	map_render(tiledit->map, &r, &src);
}

static void dtor(void *priv)
{
	struct tiledit *tiledit = priv;
	map_free(tiledit->map);
	free(tiledit);
}

static void keypress(void *priv, int key, int down)
{
	struct tiledit *tiledit = priv;
	switch(key) {
	case SDLK_ESCAPE:
	case SDLK_q:
		renderer_exit(tiledit->render, GAME_MODE_COMPLETE);
		break;
	default:
		break;
	}
}

static void mousebutton(void *priv, int button, int down)
{
	struct tiledit *tiledit = priv;

	switch(button) {
	case 1:
		if ( !down ) {
			tiledit->mouse_state = 0;
			return;
		}

		switch(tiledit->mouse_pos) {
		case POS_PALETTE:
			tiledit->mouse_state = MOUSE_TILE_DRAG;
			break;
		case POS_MAP:
			tiledit->mouse_state = MOUSE_MAP_MOVE;
			break;
		}
		break;
	default:
		return;
	}
}

static void mousemove(void *priv, unsigned int x, unsigned int y,
				int xrel, int yrel)
{
	struct tiledit *tiledit = priv;

	switch(tiledit->mouse_state) {
	case 0:
		if ( x < tiledit->lwidth )
			tiledit->mouse_pos = POS_PALETTE;
		else
			tiledit->mouse_pos = POS_MAP;
		break;
	case MOUSE_MAP_MOVE:
		scroll_to(tiledit, tiledit->mapx - xrel,
				tiledit->mapy - yrel);
		break;
	case MOUSE_TILE_DRAG:
		break;
	default:
		break;
	}
}

const struct game_ops tiledit_ops = {
	.ctor = ctor,
	.dtor = dtor,
	.render = render,
	.keypress = keypress,
	.mousebutton = mousebutton,
	.mousemove = mousemove,
};
#define TILEDIT 1

static const struct game_ops *game_modes[2] = {
	[GAME_STATE_STOPPED] NULL,
	[TILEDIT] &tiledit_ops,
};

static void mode_exit(struct _game *g, int code)
{
	switch(game_state(g)) {
	case TILEDIT:
		game_exit(g);
		break;
	default:
		abort();
	}
}

int main(int argc, char **argv)
{
	struct tiledit_common common;
	game_t g;

	if ( argc < 2 ) {
		fprintf(stderr, "Usage:\t%s <filename>\n", argv[0]);
		return EXIT_FAILURE;
	}
	common.fn = argv[1];

	g = game_new("SDL", game_modes, 2, mode_exit, &common);
	if ( NULL == g )
		return EXIT_FAILURE;

	game_set_state(g, TILEDIT);

	if ( !game_main(g) )
		return EXIT_FAILURE;

	game_free(g);
	return EXIT_SUCCESS;
}
