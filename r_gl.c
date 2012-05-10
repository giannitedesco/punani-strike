/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/punani_gl.h>
#include <punani/vec.h>
#include <punani/game.h>
#include <punani/renderer.h>
#include <punani/particles.h>
#include <punani/light.h>
#include <punani/punani_gl.h>
#include <punani/cvar.h>
#include <punani/tex.h>

#include <SDL.h>
#include <math.h>

#define RENDER_LIGHTS 1
#include "render-internal.h"
#include "tex-internal.h"

#define MAX_LIGHTS 8
struct _renderer {
	struct _light *light[MAX_LIGHTS];
	const struct tex_ops *texops;
	SDL_Surface *screen;
	game_t game;
	vec3_t viewangles;
	mat4_t view;
	unsigned int vidx, vidy;
	unsigned int vid_depth, vid_fullscreen;
	float fps;
	unsigned int vid_wireframe;
	unsigned int vid_aa;
	cvar_ns_t cvars;
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

static void swap(mat4_t mat, unsigned x, unsigned y)
{
	float tmp;
	tmp = mat[x][y];
	mat[x][y] = mat[y][x];
	mat[y][x] = tmp;
}

static void mat_transpose(mat4_t mat)
{
	swap(mat, 0, 1);
	swap(mat, 0, 2);
	swap(mat, 1, 2);
}

void renderer_xlat_world_to_obj(renderer_t r, vec3_t out, const vec3_t in)
{
	mat4_t mat, mv;

	memcpy(mat, r->view, sizeof(mat));
	mat_transpose(mat);

	glPushMatrix();
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)mv);
	glLoadMatrixf((GLfloat *)mat);
	glMultMatrixf((GLfloat *)mv);
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)mat);
	glPopMatrix();

	out[0] = v_dot_product(in, mat[0]);
	out[1] = v_dot_product(in, mat[1]);
	out[2] = v_dot_product(in, mat[2]);
}

void renderer_xlat_eye_to_obj(renderer_t r, vec3_t out, const vec3_t in)
{
	mat4_t mat;
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)mat);
	out[0] = v_dot_product(in, mat[0]);
	out[1] = v_dot_product(in, mat[1]);
	out[2] = v_dot_product(in, mat[2]);
}

void renderer_unproject(renderer_t r, vec3_t out,
			unsigned int x, unsigned int y, float h)
{
	double mvmatrix[16];
	double projmatrix[16];
	int viewport[4];
	double near[3], far[3];
	double t;
	vec3_t a, b, d;

	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);

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
	v_sub(d, a, b);
	v_normalize(d);

	t = (a[1] - h) / d[1];
	out[0] = a[0] - (d[0] * t);
	out[1] = h;
	out[2] = a[2] - (d[2] * t);
}

/* get trapezoid shape defined by frustums intersection with plane
 * parallel to ground at 'h' in current object coords.
*/
void renderer_get_frustum_quad(renderer_t r, float h, vec3_t q[4])
{
	unsigned int x, y;
	float b = 0.0;

	renderer_size(r, &x, &y);

	renderer_unproject(r, q[0], 0 + b, 0 + b, h);
	renderer_unproject(r, q[1], x - b, 0 + b, h);
	renderer_unproject(r, q[2], x - b, y - b, h);
	renderer_unproject(r, q[3], 0 + b, y - b, h);
}

static void do_render_3d(renderer_t r, int wireframe)
{
	if ( wireframe ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
	}else{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glCullFace(GL_BACK);
		glEnable(GL_LIGHTING);
	}
}

/* Prepare OpenGL for 3d rendering */
void renderer_render_3d(renderer_t r)
{
	float light[4] = {0.3, 0.3, 0.3, 1.0};

	/* Reset projection matrix */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gl_frustum(60.0f, (GLdouble)r->vidx / (GLdouble)r->vidy, 4, 4096);

	/* Reset the modelview matrix */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* Apply view transform and store it */
	glRotatef(r->viewangles[0], 1, 0, 0);
	glRotatef(r->viewangles[1], 0, 1, 0);
	glRotatef(r->viewangles[2], 0, 0, 1);
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)r->view);

	/* Finish off setting up the depth buffer */
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glClearColor(1.0, 0.0, 1.0, 1.0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light);
	glShadeModel(GL_SMOOTH);

	do_render_3d(r, r->vid_wireframe);
}

void renderer_wireframe(renderer_t r, int wireframe)
{
	do_render_3d(r, wireframe);
}

int renderer_is_wireframe(renderer_t r)
{
	return !!r->vid_wireframe;
}

void renderer_render_2d(renderer_t r)
{
	/* Use an orthogonal projection */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, r->vidx, r->vidy, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* let us wind in any direction */
	glDisable(GL_CULL_FACE);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);

	/* Enable alpha blending */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
}

void renderer_clear_color(renderer_t x, float r, float g, float b)
{
	glClearColor(r, g, b, 1.0);
}

void renderer_rotate(renderer_t r, float deg, float x, float y, float z)
{
	glRotatef(deg, x, y, z);
}

void renderer_translate(renderer_t r, float x, float y, float z)
{
	glTranslatef(x, y, z);
}

void renderer_viewangles(renderer_t r, float pitch, float roll, float yaw)
{
	r->viewangles[0] = pitch;
	r->viewangles[1] = roll;
	r->viewangles[2] = yaw;
}

void renderer_get_viewangles(renderer_t r, vec3_t angles)
{
	v_copy(angles, r->viewangles);
}

/* Global blends are done here */
int renderer_mode(renderer_t r, const char *title,
			unsigned int x, unsigned int y,
			unsigned int depth, unsigned int fullscreen)
{
	int f = SDL_OPENGL;
	int aa = r->vid_aa;

	if ( r->screen )
		SDL_Quit();

	/* Initialise SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 0;
	}

again:
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	if ( aa ) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, aa);
	}else{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}

	SDL_WM_SetCaption(title, NULL);

	SDL_EnableUNICODE(1);

	if ( fullscreen )
		f |= SDL_FULLSCREEN;

	/* Setup the SDL display */
	r->screen = SDL_SetVideoMode(x, y, depth, f);
	if ( r->screen == NULL ) {
		if ( aa ) {
			con_printf("fsaa not available\n");
			aa = 0;
			goto again;
		}
		fprintf(stderr, "SDL_SetVideoMode: %s\n", SDL_GetError());
		return 0;
	}

	/* save details for later */
	r->vidx = x;
	r->vidy = y;
	r->vid_depth = depth;
	r->vid_fullscreen = fullscreen;

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "glewInit failed: %x\n", err);
	}

	con_printf("gl_vendor: %s\n", glGetString(GL_VENDOR));
	con_printf("gl_renderer: %s\n", glGetString(GL_RENDERER));
	con_printf("gl_version: %s\n", glGetString(GL_VERSION));
	con_printf("extensions: %s\n", glGetString(GL_EXTENSIONS));

	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_MULTISAMPLE);

	r->fps = 30.0;

	return 1;
}

float renderer_fps(renderer_t r)
{
	return r->fps;
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

static void tex_unbind(struct _texture *tex)
{
	if ( tex->t_u.gl.uploaded ) {
		glDeleteTextures(1, &tex->t_u.gl.texnum);
		tex->t_u.gl.uploaded = 0;
	}
}

void texture_bind(texture_t tex)
{
	tex_bind(tex);
}

void renderer_blit(renderer_t r, texture_t tex, prect_t *src, prect_t *dst)
{
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

void renderer_size(renderer_t r, unsigned int *x, unsigned int *y)
{
	if ( x )
		*x = r->vidx;
	if ( y )
		*y = r->vidy;
}

static void render_begin(void)
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
}

static void render_end(void)
{
	glFlush();
	SDL_GL_SwapBuffers();
}

int renderer_main(renderer_t r)
{
	SDL_Event e;
	uint32_t now, nextframe = 0, gl_frames = 0;
	uint32_t ctr;
	float lerp;
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
						(e.type == SDL_KEYDOWN), e.key);
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
		if ( (gl_frames % 10) == 0 ) {
			r->fps = 10000.0f / (now - ctr);
			ctr = now;
			//con_printf("%f fps\n", r->fps);
		}
	}

	game_free(g);

	return EXIT_SUCCESS;
}

void renderer_exit(renderer_t r, int code)
{
	game_mode_exit(r->game, code);
}

static int t_rgba(struct _texture *t, unsigned int x, unsigned int y)
{
	t->t_u.gl.buf = malloc(x * y * 4);
	if ( NULL == t->t_u.gl.buf )
		return 0;
	t->t_u.gl.width = x;
	t->t_u.gl.height = y;
	t->t_u.gl.format = GL_RGBA;
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
		tex_unbind(t);
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

renderer_t renderer_new(game_t g)
{
	struct _renderer *r = NULL;

	r = calloc(1, sizeof(*r));
	if ( NULL == r )
		return r;

	r->game = g;
	r->texops = &tex_gl;

	r->cvars = cvar_ns_new("render");

	cvar_register_uint(r->cvars, "wireframe",
				CVAR_FLAG_SAVE_NOTDEFAULT,
				&r->vid_wireframe);
	cvar_register_uint(r->cvars, "aa",
				CVAR_FLAG_SAVE_NOTDEFAULT,
				&r->vid_aa);
	cvar_ns_load(r->cvars);

	particles_init();

	return r;
}

void renderer_free(renderer_t r)
{
	if ( r ) {
		cvar_ns_save(r->cvars);
		cvar_ns_free(r->cvars);
		particles_exit();
		SDL_Quit();
		free(r);
	}
}

const struct tex_ops *renderer_texops(struct _renderer *r)
{
	return r->texops;
}

int renderer_get_free_light(renderer_t r)
{
	unsigned int i;

	for(i = 0; i < MAX_LIGHTS; i++) {
		if ( NULL == r->light[i] )
			return i;
	}

	return -1;
}

void renderer_set_light(renderer_t r, unsigned int num, light_t l)
{
	r->light[num] = l;
}

void renderer_nuke_light(renderer_t r, unsigned int num)
{
	assert(num < MAX_LIGHTS);
	r->light[num] = NULL;
}
