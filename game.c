#include <punani/punani.h>
#include <punani/game.h>

struct _game {
	unsigned int g_state;
};

game_t game_new(void)
{
	struct _game *g;

	g = calloc(1, sizeof(*g));
	if ( NULL == g )
		return NULL;

	g->g_state = GAME_STATE_LOBBY;
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
}
