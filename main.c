/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/game.h>
#include <punani/tex.h>

#include "render-internal.h"
#include "tex-internal.h"

static unsigned int vidx, vidy;
static unsigned int vid_depth, vid_fullscreen;
static SDL_Surface *screen;

static int r_init(void *priv, unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen)
{
	int f = 0;

	/* Initialise SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 0;
	}

	SDL_WM_SetCaption("Punani Strike", NULL);

	/* Cleanup SDL on exit */
	atexit(SDL_Quit);

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
	screen = SDL_SetVideoMode(x, y, depth, f);
	if ( screen == NULL ) {
		fprintf(stderr, "SDL_SetVideoMode: %s\n", SDL_GetError());
		return 0;
	}

	/* save details for later */
	vidx = x;
	vidy = y;
	vid_depth = depth;
	vid_fullscreen = fullscreen;

	return 1;
}

static void r_blit(void *priv, texture_t tex, prect_t *src, prect_t *dst)
{
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

	SDL_BlitSurface(texture_surface(tex), sp, screen, dp);
}

static void r_size(void *priv, unsigned int *x, unsigned int *y)
{
	if ( x )
		*x = vidx;
	if ( y )
		*y = vidy;
}

int main(int argc, char **argv)
{
	static const struct render_ops rops = {
		.blit = r_blit,
		.size = r_size,
		.exit = game_mode_exit,
		.init = r_init,
	};
	struct _renderer render;
	SDL_Event e;
	uint32_t now, nextframe = 0, gl_frames = 0;
	uint32_t ctr;
	float lerp;
	float fps = 30.0;
	game_t g;

	render.ops = &rops;
	render.priv = NULL;

	g = game_new(&render);
	if ( NULL == g ) {
		fprintf(stderr, "failed to create game\n");
		return EXIT_FAILURE;
	}

	if ( !game_lobby(g) ) {
		fprintf(stderr, "failed to open lobby\n");
		return EXIT_FAILURE;
	}

	render.priv = g;
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
		SDL_FillRect(screen, NULL, 0);
		game_render(g, lerp);
		SDL_Flip(screen);
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
