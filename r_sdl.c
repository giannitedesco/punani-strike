/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/game.h>
#include <punani/renderer.h>
#include <punani/tex.h>

#include "render-internal.h"
#include "tex-internal.h"

struct r_sdl {
	unsigned int vidx, vidy;
	unsigned int vid_depth, vid_fullscreen;
	SDL_Surface *screen;
	game_t game;
};

static int r_mode(void *priv, unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen)
{
	struct r_sdl *r = priv;
	int f = 0;

	if ( r->screen )
		SDL_Quit();

	/* Initialise SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 0;
	}

	SDL_WM_SetCaption("Punani Strike", NULL);

	if ( fullscreen )
		f |= SDL_FULLSCREEN;

	/* Need 5 bits of color depth for each color */
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	/* Enable double buffering */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	/* Setup the depth buffer, 16 deep */
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

	/* Setup the SDL display */
	r->screen = SDL_SetVideoMode(x, y, depth, f);
	if ( r->screen == NULL ) {
		fprintf(stderr, "SDL_SetVideoMode: %s\n", SDL_GetError());
		return 0;
	}

	/* save details for later */
	r->vidx = x;
	r->vidy = y;
	r->vid_depth = depth;
	r->vid_fullscreen = fullscreen;

	return 1;
}

static void r_blit(void *priv, texture_t tex, prect_t *src, prect_t *dst)
{
	struct r_sdl *r = priv;
	SDL_Rect s, d, *sp, *dp;

	sp = dp = NULL;

	if ( src ) {
		s.x = src->x;
		s.y = src->y;
		s.w = src->w;
		s.h = src->h;
		sp = &s;
	}

	if ( dst ) {
		d.x = dst->x;
		d.y = dst->y;
		d.w = dst->w;
		d.h = dst->h;
		dp = &d;
	}

	SDL_BlitSurface(texture_surface(tex), sp, r->screen, dp);
}

static void r_size(void *priv, unsigned int *x, unsigned int *y)
{
	struct r_sdl *r = priv;
	if ( x )
		*x = r->vidx;
	if ( y )
		*y = r->vidy;
}

static int r_main(void *priv)
{
	struct r_sdl *r = priv;
	SDL_Event e;
	uint32_t now, nextframe = 0, gl_frames = 0;
	uint32_t ctr;
	float lerp;
	float fps = 30.0;
	game_t g = r->game;

	now = ctr = SDL_GetTicks();

	while( game_state(g) != GAME_STATE_STOPPED ) {
		/* poll for client input events */
		while( SDL_PollEvent(&e) ) {
			switch ( e.type ) {
			case SDL_MOUSEMOTION:
				game_mousemove(g, e.motion.x,
						e.motion.y,
						e.motion.xrel,
						e.motion.yrel);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				game_keypress(g, e.key.keysym.sym,
						(e.type == SDL_KEYDOWN));
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				game_mousebutton(g, e.button.button,
					(e.type == SDL_MOUSEBUTTONDOWN));
				break;
			case SDL_QUIT:
				game_exit(g);
				break;
			default:
				break;
			}
		}

		now = SDL_GetTicks();

		/* Run client frames */
		if ( now >= nextframe ) {
			nextframe = now + 100;
			lerp = 0;
			game_new_frame(g);
		}else{
			lerp = 1.0f - ((float)nextframe - now)/100.0f;
			if ( lerp > 1.0 )
				lerp = 1.0;
		}

		/* Render a scene */
		SDL_FillRect(r->screen, NULL, 0);
		game_render(g, lerp);
		SDL_Flip(r->screen);
		gl_frames++;

		/* Calculate FPS */
		if ( (gl_frames % 100) == 0 ) {
			fps = 100000.0f / (now - ctr);
			ctr = now;
			//printf("%f fps\n", fps);
		}
	}

	game_free(g);

	return EXIT_SUCCESS;
}

static void r_exit(void *priv, int code)
{
	struct r_sdl *r = priv;
	game_mode_exit(r->game, code);
}

static int r_ctor(struct _renderer *renderer, struct _game *g)
{
	struct r_sdl *r = NULL;

	r = calloc(1, sizeof(*r));
	if ( NULL == r )
		return 0;

	r->game = g;

	renderer->priv = r;
	return 1;
}

static void r_dtor(void *priv)
{
	struct r_sdl *r = priv;
	if ( r ) {
		SDL_Quit();
		free(r);
	}
}

const struct render_ops render_sdl = {
	.blit = r_blit,
	.size = r_size,
	.exit = r_exit,
	.mode = r_mode,
	.main = r_main,
	.ctor = r_ctor,
	.dtor = r_dtor,
};
