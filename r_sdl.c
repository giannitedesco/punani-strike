/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/game.h>
#include <punani/renderer.h>
#include <punani/tex.h>

#include <SDL.h>
#include "render-internal.h"
#include "tex-internal.h"

struct r_sdl {
	unsigned int vidx, vidy;
	unsigned int vid_depth, vid_fullscreen;
	SDL_Surface *screen;
	game_t game;
};

static int r_mode(void *priv, const char *title,
			unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen)
{
	struct r_sdl *r = priv;
	int f = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_DOUBLEBUF;

	if ( r->screen )
		SDL_Quit();

	/* Initialise SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 0;
	}

	SDL_WM_SetCaption(title, NULL);

	if ( fullscreen )
		f |= SDL_FULLSCREEN;

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

	SDL_BlitSurface(tex->t_u.sdl.surf, sp, r->screen, dp);
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

static SDL_Surface *new_surface(unsigned int x, unsigned int y, int alpha)
{
	SDL_Surface *surf;
	Uint32 rmask, gmask, bmask, amask;
	Uint32 flags = SDL_HWSURFACE;

	if ( alpha ) {
		flags |= SDL_SRCALPHA;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
#endif

	}else{
		flags |= SDL_SRCALPHA | SDL_SRCCOLORKEY;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0;
#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0;
#endif

	}

	surf = SDL_CreateRGBSurface(flags, x, y, (alpha) ? 32 : 24,
					rmask, gmask, bmask, amask);
	if (surf == NULL)
		return NULL;

	if ( !alpha ) {
		SDL_SetColorKey(surf,
				SDL_RLEACCEL | SDL_SRCALPHA | SDL_SRCCOLORKEY,
				SDL_MapRGB(surf->format, 0xff, 0, 0xff));
	}
	return surf;
}

static int t_rgba(struct _texture *t, unsigned int x, unsigned int y)
{
	t->t_u.sdl.surf = new_surface(x, y, 1);
	if ( NULL == t->t_u.sdl.surf )
		return 0;
	return 1;
}

static int t_rgb(struct _texture *t, unsigned int x, unsigned int y)
{
	t->t_u.sdl.surf = new_surface(x, y, 0);
	if ( NULL == t->t_u.sdl.surf )
		return 0;
	return 1;
}

static void t_lock(struct _texture *t)
{
	SDL_LockSurface(t->t_u.sdl.surf);
}

static void t_unlock(struct _texture *t)
{
	SDL_UnlockSurface(t->t_u.sdl.surf);
}

static void t_free(struct _texture *t)
{
	if ( t->t_u.sdl.surf ) {
		SDL_FreeSurface(t->t_u.sdl.surf);
		t->t_u.sdl.surf = NULL;
	}
}

static uint8_t *t_pixels(struct _texture *t)
{
	SDL_Surface *surf = t->t_u.sdl.surf;
	return surf->pixels;
}

static const struct tex_ops tex_sdl = {
	.alloc_rgba = t_rgba,
	.alloc_rgb = t_rgb,
	.lock = t_lock,
	.pixels = t_pixels,
	.unlock = t_unlock,
	.free = t_free,
};

const struct render_ops render_sdl = {
	.blit = r_blit,
	.size = r_size,
	.exit = r_exit,
	.mode = r_mode,
	.main = r_main,
	.ctor = r_ctor,
	.dtor = r_dtor,
	.texops = &tex_sdl,
};
