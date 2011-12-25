#include <punani/punani.h>
#include <punani/game.h>

int main(int argc, char **argv)
{
	game_t game;
	SDL_Event e;
	uint32_t now, nextframe = 0, gl_frames = 0;
	uint32_t ctr;
	float lerp;
	float fps = 30.0;

	game = game_new();
	if ( NULL == game ) {
		fprintf(stderr, "failed to create game\n");
		return EXIT_FAILURE;
	}

	/* Initialise SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	/* Cleanup SDL on exit */
	atexit(SDL_Quit);

	now = ctr = SDL_GetTicks();

	while( game_state(game) != GAME_STATE_STOPPED ) {
		/* poll for client input events */
		while( SDL_PollEvent(&e) ) {
			switch ( e.type ) {
			case SDL_MOUSEMOTION:
				//sdl_mouse_event(&e);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				//sdl_keyb_event(&e);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				//sdl_keyb_mouse(&e);
				break;
			case SDL_QUIT:
				game_exit(game);
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
			game_new_frame(game);
		}else{
			lerp = 1.0f - ((float)nextframe - now)/100.0f;
			if ( lerp > 1.0 )
				lerp = 1.0;
		}

		/* Render a scene */
		game_render(game, lerp);
		gl_frames++;

		/* Calculate FPS */
		if ( (gl_frames % 100) == 0 ) {
			fps = 100000.0f / (now - ctr);
			ctr = now;
		}
	}

	game_free(game);

	return EXIT_SUCCESS;
}
