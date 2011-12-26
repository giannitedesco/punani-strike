#include <punani/punani.h>
#include <punani/game.h>
#include <punani/tex.h>

struct _game {
	unsigned int g_state;
	unsigned int g_vidx;
	unsigned int g_vidy;
	unsigned int g_vid_depth;
	unsigned int g_vid_fullscreen;
	SDL_Surface *g_screen;

	texture_t g_map;
};

static int init_sdl(void)
{
	/* Initialise SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 0;
	}

	/* Cleanup SDL on exit */
	atexit(SDL_Quit);
	return 1;
}

static int init_vid(struct _game *g)
{
	int f = 0;

	if ( g->g_vid_fullscreen )
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
	g->g_screen = SDL_SetVideoMode(g->g_vidx, g->g_vidy, g->g_vid_depth, f);
	if ( g->g_screen == NULL ) {
		fprintf(stderr, "SDL_SetVideoMode: %s\n", SDL_GetError());
		return 0;
	}

	return 1;
}

game_t game_new(void)
{
	struct _game *g;

	g = calloc(1, sizeof(*g));
	if ( NULL == g )
		goto out;

	if ( !init_sdl() )
		goto out_free;

	g->g_vidx = 960;
	g->g_vidy = 540;
	g->g_vid_depth = 24;
	if ( !init_vid(g) )
		goto out_free;

	/* success */
	g->g_state = GAME_STATE_LOBBY;
	goto out;

out_free:
	free(g);
	g = NULL;
out:
	return g;
}

unsigned int game_state(game_t g)
{
	return g->g_state;
}

void game_exit(game_t g)
{
	g->g_state = GAME_STATE_STOPPED;
}

int game_start(game_t g)
{
	g->g_state = GAME_STATE_ON;
	g->g_map = png_get_by_name("data/map1.png");
	return 1;
}

void game_free(game_t g)
{
	if ( g ) {
		free(g);
	}
}

/* one tick has elapsed in game time. the game tick interval
 * is clamped to real time so we can increment the emulation
 * as accurately as possible to wall time
*/
void game_new_frame(game_t g)
{
	printf("frame\n");
}

/* lerp is a value clamped between 0 and 1 which indicates how far between
 * game ticks we are. render times may fluctuate but we are called to
 * render as fast as possible
*/
void game_render(game_t g, float lerp)
{
	SDL_FillRect(g->g_screen, NULL, 0);
	SDL_BlitSurface(texture_surface(g->g_map), NULL,
					g->g_screen, NULL);
	SDL_Flip(g->g_screen);
}

void game_keypress(game_t g, int key, int down)
{
}

void game_mousbutton(game_t g, int button, int down)
{
}

void game_mousemove(game_t g, int xrel, int yrel)
{
}
