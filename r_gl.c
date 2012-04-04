/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/game.h>
#include <punani/renderer.h>
#include <punani/tex.h>

#include <SDL.h>
#include <GL/gl.h>
#include <math.h>

#include "render-internal.h"
#include "tex-internal.h"

struct r_gl {
	unsigned int vidx, vidy;
	unsigned int vid_depth, vid_fullscreen;
	int vid_wireframe;
	SDL_Surface *screen;
	game_t game;
};

/* Help us to setup the viewing frustum */
static void gl_frustum(GLdouble fovy,
		GLdouble aspect,
		GLdouble zNear,
		GLdouble zFar)
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

/* Prepare OpenGL for 3d rendering */
static void gl_init_3d(struct r_gl *r)
{
	/* Reset projection matrix */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gl_frustum(90.0f, (GLdouble)r->vidx / (GLdouble)r->vidy, 4, 4069);

	/* Reset the modelview matrix */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* Finish off setting up the depth buffer */
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	/* Use back-face culling */
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	if ( r->vid_wireframe ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
	}else{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_TEXTURE_2D);
	}
}

static void gl_init_2d(struct r_gl *r)
{
	/* Use an orthogonal projection */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, r->vidx, r->vidy, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	/* So we can wind in any direction */
	glDisable(GL_CULL_FACE);

	if ( r->vid_wireframe ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
	}else{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_TEXTURE_2D);
	}
}

/* Global blends are done here */
static int r_mode(void *priv, const char *title,
			unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen)
{
	struct r_gl *r = priv;
	int f = SDL_OPENGL;

	if ( r->screen )
		SDL_Quit();

	/* Initialise SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 0;
	}

	/* Need 8 bits of color depth for each color */
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	/* Enable double buffering */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	/* Setup the depth buffer, 16 deep */
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

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

	printf("gl_vendor: %s\n", glGetString(GL_VENDOR));
	printf("gl_renderer: %s\n", glGetString(GL_RENDERER));
	printf("gl_version: %s\n", glGetString(GL_VERSION));
	printf("extensions: %s\n", glGetString(GL_EXTENSIONS));

	glClearColor(0, 0, 0, 1);

	r->vid_wireframe = 0;
#if 0
	gl_init_3d(r);
#else
	gl_init_2d(r);
#endif

	return 1;
}

static void tex_upload(struct _texture *tex)
{
	glBindTexture(GL_TEXTURE_2D, tex->t_u.gl.texnum);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		tex->t_u.gl.width,
		tex->t_u.gl.height,
		0,
		tex->t_u.gl.format,
		GL_UNSIGNED_BYTE,
		tex->t_u.gl.buf);
}

static void tex_bind(struct _texture *tex)
{
	if ( !tex->t_u.gl.uploaded ) {
		glGenTextures(1, &tex->t_u.gl.texnum);
		tex_upload(tex);
		tex->t_u.gl.uploaded = 1;
	}
	glBindTexture(GL_TEXTURE_2D, tex->t_u.gl.texnum);
}

#if 0
static void tex_unbind(struct _texture *tex)
{
	if ( tex->t_u.gl.uploaded ) {
		glDeleteTextures(1, &tex->t_u.gl.texnum);
		tex->t_u.gl.uploaded = 0;
	}
}
#endif

static void r_blit(void *priv, texture_t tex, prect_t *src, prect_t *dst)
{
	//struct r_gl *r = priv;
	struct {
		float x, y;
		float w, h;
	}s;

	if ( src ) {
		s.x = (float)src->x / (float)tex->t_u.gl.width;
		s.y = (float)src->y / (float)tex->t_u.gl.height;
		s.w = (float)src->w / (float)tex->t_u.gl.width;
		s.h = (float)src->h / (float)tex->t_u.gl.width;
	}else{
		s.x = s.y = 0.0;
		s.w = 1.0;
		s.h = 1.0;
	}

	tex_bind(tex);
	glBegin(GL_QUADS);
	glTexCoord2f(s.x, s.y);
	glVertex2i(dst->x, dst->y);
	glTexCoord2f(s.x + s.w, s.y);
	glVertex2i(dst->x + dst->w, dst->y);
	glTexCoord2f(s.x + s.w, s.y + s.h);
	glVertex2i(dst->x + dst->w, dst->y + dst->h);
	glTexCoord2f(s.x, s.y + s.h);
	glVertex2i(dst->x, dst->y + dst->h);
	glEnd();
}

static void r_size(void *priv, unsigned int *x, unsigned int *y)
{
	struct r_gl *r = priv;
	if ( x )
		*x = r->vidx;
	if ( y )
		*y = r->vidy;
}

static void render_begin(void)
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

static void render_end(void)
{
	SDL_GL_SwapBuffers();
}

static int r_main(void *priv)
{
	struct r_gl *r = priv;
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
		render_begin();
		game_render(g, lerp);
		render_end();
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
	struct r_gl *r = priv;
	game_mode_exit(r->game, code);
}

static int r_ctor(struct _renderer *renderer, struct _game *g)
{
	struct r_gl *r = NULL;

	r = calloc(1, sizeof(*r));
	if ( NULL == r )
		return 0;

	r->game = g;

	renderer->priv = r;
	return 1;
}

static void r_dtor(void *priv)
{
	struct r_gl *r = priv;
	if ( r ) {
		SDL_Quit();
		free(r);
	}
}

static int t_rgba(struct _texture *t, unsigned int x, unsigned int y)
{
	/* FIXME: alpha keying */
	t->t_u.gl.buf = malloc(x * y * 3);
	if ( NULL == t->t_u.gl.buf )
		return 0;
	t->t_u.gl.width = x;
	t->t_u.gl.height = y;
	t->t_u.gl.format = GL_RGB;
	return 1;
}

static int t_rgb(struct _texture *t, unsigned int x, unsigned int y)
{
	t->t_u.gl.buf = malloc(x * y * 3);
	if ( NULL == t->t_u.gl.buf )
		return 0;
	t->t_u.gl.width = x;
	t->t_u.gl.height = y;
	t->t_u.gl.format = GL_RGB;
	return 1;
}

static void t_free(struct _texture *t)
{
	if ( t->t_u.gl.buf ) {
		free(t->t_u.gl.buf);
		t->t_u.gl.buf = NULL;
	}
}

static uint8_t *t_pixels(struct _texture *t)
{
	return t->t_u.gl.buf;
}

static const struct tex_ops tex_gl = {
	.alloc_rgba = t_rgba,
	.alloc_rgb = t_rgb,
	.pixels = t_pixels,
	.free = t_free,
};

const struct render_ops render_gl = {
	.blit = r_blit,
	.size = r_size,
	.exit = r_exit,
	.mode = r_mode,
	.main = r_main,
	.ctor = r_ctor,
	.dtor = r_dtor,
	.texops = &tex_gl,
};
