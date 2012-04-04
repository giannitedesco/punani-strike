/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/game.h>
#include <punani/renderer.h>
#include <punani/map.h>

#include "render-internal.h"
#include "dessert-stroke.h"
#include "game-modes.h"

#include <SDL/SDL_keysym.h>

struct tiledit_common {
	const char *fn;
};

#define LEFT_CLICK		(1 << 0)
#define RIGHT_CLICK		(1 << 1)

#define STATE_GO		0
#define STATE_SAVE_CHANGES	1

struct tiledit {
	renderer_t render;

	const char *fn;
	map_t map;

	unsigned int state;
	texture_t save_changes;
	texture_t tiles;

	int tile_id;
	unsigned int tw, th;
	unsigned int mapw, maph;
	int mapx, mapy;
	unsigned int mouse_state;
	unsigned int lwidth;

	/* current mouse pos, pos when left/right buttons were clicked */
	unsigned int mposx, mposy;
	unsigned int rposx, rposy;
	unsigned int lposx, lposy;
};

static void *ctor(renderer_t r, void *priv)
{
	struct tiledit_common *common = priv;
	struct tiledit *tiledit = NULL;

	tiledit = calloc(1, sizeof(*tiledit));
	if ( NULL == tiledit )
		return NULL;

	tiledit->render = r;

	tiledit->fn = common->fn;
	tiledit->map = map_load(r, common->fn);
	tiledit->tile_id = -1;
	if ( NULL == tiledit->map )
		goto out_free;

	map_get_size(tiledit->map, &tiledit->mapw, &tiledit->maph);
	tiledit->tiles = map_get_tiles(tiledit->map,
					&tiledit->tw, &tiledit->th);
	//tiledit->lwidth = texture_width(tiledit->tiles);

	tiledit->save_changes = png_get_by_name(r, "data/save-changes.png", 0);
	if ( NULL == tiledit->save_changes )
		goto out_free_map;

	/* success */
	goto out;

out_free_map:
	map_free(tiledit->map);
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

static void render_save_changes(struct tiledit *tiledit)
{
	unsigned int sx, sy, x, y;
	prect_t dst;

	renderer_size(tiledit->render, &x, &y);
	sx = texture_width(tiledit->save_changes);
	sy = texture_height(tiledit->save_changes);

	dst.x = (x - sx) / 2;
	dst.y = (y - sy) / 2;
	dst.w = sx;
	dst.h = sy;

	renderer_blit(tiledit->render, tiledit->save_changes, NULL, &dst);
}

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

	if ( tiledit->state == STATE_SAVE_CHANGES )
		render_save_changes(tiledit);
}

static void dtor(void *priv)
{
	struct tiledit *tiledit = priv;
	texture_put(tiledit->save_changes);
	map_free(tiledit->map);
	free(tiledit);
}

static void keypress(void *priv, int key, int down)
{
	struct tiledit *tiledit = priv;
	switch(key) {
	case SDLK_ESCAPE:
	case SDLK_q:
		if ( down ) {
			if ( tiledit->state == STATE_GO )
				tiledit->state = STATE_SAVE_CHANGES;
			else
				tiledit->state = STATE_GO;
		}
		break;
	case SDLK_y:
		map_save(tiledit->map, tiledit->fn);
		printf("saved to %s\n", tiledit->fn);
	case SDLK_n:
		renderer_exit(tiledit->render, GAME_MODE_COMPLETE);
	default:
		break;
	}
}

static void mousebutton(void *priv, int button, int down)
{
	struct tiledit *tiledit = priv;
	unsigned int flag, x, y;

	x = tiledit->mposx;
	y = tiledit->mposy;

	switch(button) {
	case 1:
		flag = LEFT_CLICK;
		if ( down ) {
			tiledit->lposx = x;
			tiledit->lposy = y;
		}else if ( tiledit->lposx == x && 
				tiledit->lposy == y && 
				x >= tiledit->lwidth ){
			map_set_tile_at(tiledit->map,
						(x - tiledit->lwidth) +
							tiledit->mapx,
						y + tiledit->mapy,
						tiledit->tile_id);
		}
		break;
	case 3:
		flag = RIGHT_CLICK;
		if ( down ) {
			tiledit->rposx = x;
			tiledit->rposy = y;
		}else if ( tiledit->rposx == x && 
				tiledit->rposy == y && 
				x >= tiledit->lwidth ){
			tiledit->tile_id = map_tile_at(tiledit->map,
						(x - tiledit->lwidth) + 
							tiledit->mapx,
						y + tiledit->mapy);
		}
		break;
	default:
		return;
	}

	if ( down )
		tiledit->mouse_state |= flag;
	else
		tiledit->mouse_state &= ~flag;
}

static void mousemove(void *priv, unsigned int x, unsigned int y,
				int xrel, int yrel)
{
	struct tiledit *tiledit = priv;

	tiledit->mposx = x;
	tiledit->mposy = y;

	switch(tiledit->mouse_state) {
	case LEFT_CLICK:
		if ( tiledit->lposx >= tiledit->lwidth ) {
			scroll_to(tiledit, tiledit->mapx - xrel,
					tiledit->mapy - yrel);
		}
		break;
	case RIGHT_CLICK:
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
	char buf[512];
	game_t g;

	if ( argc < 2 ) {
		fprintf(stderr, "Usage:\t%s <filename>\n", argv[0]);
		return EXIT_FAILURE;
	}
	common.fn = argv[1];

	g = game_new("SDL", game_modes, 2, mode_exit, &common);
	if ( NULL == g )
		return EXIT_FAILURE;

	snprintf(buf, sizeof(buf), "Punani Map Editor (%s)", common.fn);
	if ( !game_mode(g, buf, 960, 540, 24, 0) )
		return EXIT_FAILURE;
	game_set_state(g, TILEDIT);

	if ( !game_main(g) )
		return EXIT_FAILURE;

	game_free(g);
	return EXIT_SUCCESS;
}
