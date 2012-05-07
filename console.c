#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <SDL.h>
#include <punani/punani.h>
#include <punani/punani_gl.h>
#include <punani/console.h>


/* max line len excluding null terminator */
#define CONSOLE_LINE_MAX_LEN 1023
#define CONSOLE_MAX_LINES 100

#define CONSOLE_VISIBLE 1
/* todo: #define CONSOLE_MOVING :) */
#define CONSOLE_HIDDEN 16

struct _console {
	/* one of CONSOLE_VISIBLE or CONSOLE_HIDDEN */
	int  state;
	
	/* initialised in con_init, maths for console display uses this */
	int  width;
	int  height;
	
	font_t font;
	texture_t conback;

    /* line log stuff */
	char lines[CONSOLE_MAX_LINES][CONSOLE_LINE_MAX_LEN + 1];
	int  line;

    /* line input stuff */
	int  cursor_offs;
	char input_line[CONSOLE_LINE_MAX_LEN + 1];
};

console_t con_default = NULL;

void con_init(font_t font,  texture_t conback, const int screen_width, const int screen_height) {
	con_default = (console_t) calloc(1, sizeof(*con_default));
	con_default->state = CONSOLE_HIDDEN;
	assert( NULL != font );
	con_default->font = font;
	con_default->conback = conback;
	con_default->width = screen_width;
	con_default->height = screen_height;
	con_printf("punani strike console");
	if (con_default->conback) {
		con_printf("using conback (%d x %d)", texture_height(con_default->conback), texture_width(con_default->conback));
	}
}

void con_printf(const char *fmt, ...) {
	va_list args;
	
	if ( NULL == con_default ) return;
	
	va_start(args, fmt);
	vsnprintf(con_default->lines[con_default->line++], CONSOLE_LINE_MAX_LEN, fmt, args);
	va_end(args);
	con_default->line = con_default->line % CONSOLE_MAX_LINES;
}

void con_input_splice(int s0_start, int s0_end, int s1_start);
void con_input_insert(char c);

/* splice some characters out of the current input line. current input line ends up consisting of line[s0_start]..line[s0_end] + line[s1_start]..line[CONSOLE_LINE_MAX_LEN]. */
void con_input_splice(int s0_start, int s0_end, int s1_start) {
	
	assert(s0_end < CONSOLE_LINE_MAX_LEN);
	assert(s0_start < CONSOLE_LINE_MAX_LEN);
	
	char newline[CONSOLE_LINE_MAX_LEN];
	int s0_len = s0_end - s0_start;
	
	strncpy(newline, &con_default->input_line[s0_start], s0_len);
	strncpy(&newline[s0_len], &con_default->input_line[s1_start], CONSOLE_LINE_MAX_LEN - s1_start);
	
	strncpy(con_default->input_line, newline, CONSOLE_LINE_MAX_LEN);
}

/* add the typed char into the string where the cursor is. */
void con_input_insert(char c) {
	if (con_default->cursor_offs == CONSOLE_LINE_MAX_LEN - 1) return;
	
	if (con_default->input_line[con_default->cursor_offs] == '\0') {
		con_default->input_line[con_default->cursor_offs] = c;
		++con_default->cursor_offs;
		con_default->input_line[con_default->cursor_offs] = '\0';
	} else {
		char newline[CONSOLE_LINE_MAX_LEN];
		strncpy(newline, con_default->input_line, con_default->cursor_offs);
		strncpy(&newline[con_default->cursor_offs + 1], &con_default->input_line[con_default->cursor_offs], CONSOLE_LINE_MAX_LEN - con_default->cursor_offs);
		newline[con_default->cursor_offs] = c;
		
		strncpy(con_default->input_line, newline, CONSOLE_LINE_MAX_LEN);
		con_default->cursor_offs++;
	}
}

int con_keypress(int key, int down, void *raw) {
	if ( NULL == con_default ) goto out_unused_key;
	if ( !down ) goto out_unused_key;
		
	SDL_KeyboardEvent *event = (SDL_KeyboardEvent *) raw;
	
	if ( SDLK_BACKQUOTE == key ) {
		if ( CONSOLE_HIDDEN == con_default->state ) {
			con_default->state = CONSOLE_VISIBLE;
			SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		} else { 
			con_default->state = CONSOLE_HIDDEN;
			SDL_EnableKeyRepeat(0, 0);
		}
		goto out_used_key;
	}

	if ( CONSOLE_VISIBLE == con_default->state ) {
		
		/* todo: capture the key, do the line input, all that jazz */
		switch(event->keysym.sym) {
		case SDLK_BACKSPACE:
			if (con_default->cursor_offs > 0) {
				con_input_splice(0, con_default->cursor_offs - 1, con_default->cursor_offs);
				con_default->cursor_offs--;
			}
			break;
		case SDLK_DELETE:
			if (con_default->input_line[con_default->cursor_offs] != '\0') {
				con_input_splice(0, con_default->cursor_offs, con_default->cursor_offs + 1);
			}
			break;
		case SDLK_LEFT:
			if (event->keysym.mod & KMOD_CTRL) {
				if (con_default->cursor_offs > 0) {
					/* handle the case where the cursor is on the 'a' in 'bbb a'  */
					if (con_default->input_line[con_default->cursor_offs - 1] == ' ') {
						con_default->cursor_offs--;
					}
					
					/* temp replace the current char with a null terminator, find the last space, jump to it. */
					char tmp = con_default->input_line[con_default->cursor_offs];
					con_default->input_line[con_default->cursor_offs] = '\0';
					char *pos = strrchr(con_default->input_line, ' ');
					con_default->input_line[con_default->cursor_offs] = tmp;
					if ( NULL == pos ) {
						/* just default to the end of the line. */
						con_default->cursor_offs = 0;
					} else {
						con_default->cursor_offs = (int)(pos - con_default->input_line) + 1;
					}
				}
			} else {
				if (con_default->cursor_offs > 0) con_default->cursor_offs--;
			}
			break;
		case SDLK_RIGHT:
			if (event->keysym.mod & KMOD_CTRL) {
				char *pos = strchr(&con_default->input_line[con_default->cursor_offs], ' ');
				if ( NULL == pos ) {
					/* just default to the end of the line. */
					con_default->cursor_offs = strlen(con_default->input_line);
				} else {
					con_default->cursor_offs = (int)(pos - con_default->input_line) + 1;
				}
			} else {
				if (con_default->cursor_offs < CONSOLE_LINE_MAX_LEN && con_default->input_line[con_default->cursor_offs] != '\0') con_default->cursor_offs++;
			}
			break;
		case SDLK_HOME:
			con_default->cursor_offs = 0;
			break;
		case SDLK_END:
			con_default->cursor_offs = strlen(con_default->input_line);
			break;
		case SDLK_RETURN:
			con_printf(con_default->input_line);
			con_default->cursor_offs = 0;
			con_default->input_line[0] = '\0';
			break;
		default:
			if (key >= 0x20 && key <= 0x7F) {
				con_input_insert((char)event->keysym.unicode);
			}
		}
		
		
		goto out_used_key;
	}
	
	
	goto out_unused_key;
	
	
/* this layout is for you gianni */
out_used_key:
	return 1;

out_unused_key:
	return 0;
}

void con_render(void) {
	if ( NULL == con_default ) return;
		
		
	if ( CONSOLE_HIDDEN == con_default->state) {
		/* todo: draw the "last 4 lines" */
	} else {
		float pitchx, pitchy;
		int i, offs, cursor_screen_offs;

		font_get_pitch(con_default->font, &pitchx, &pitchy);
		
		int const num_lines = 10;
		int const visible_height = pitchy * num_lines;
		int const border_top = con_default->height - visible_height - 5;
		
		if (con_default->conback) {
			glEnable(GL_TEXTURE_2D);
			texture_bind(con_default->conback);
			/* try toggling disabling blend - looks nice in different ways in each setting */
			/* glDisable(GL_BLEND); */
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glBegin(GL_QUADS);
			glColor4f(1,1,1,1); 
			glTexCoord2f(0, 0); glVertex2i(0, border_top);
			glTexCoord2f(1, 0); glVertex2i(con_default->width, border_top);
			glTexCoord2f(1, 1); glVertex2i(con_default->width, con_default->height);
			glTexCoord2f(0, 1); glVertex2i(0, con_default->height);
			glEnd();
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);
			glColor4f(0, 1.f, 0, 0.f); 
			glVertex2i(0, border_top);
			glVertex2i(con_default->width, border_top);
			glVertex2i(con_default->width, con_default->height);
			glVertex2i(0, con_default->height);
			glEnd();
			glEnable(GL_TEXTURE_2D);
		}

		for(i = con_default->line - 1, offs = 1; i >= 0 && offs < num_lines; i--, offs++) {
			font_print(con_default->font, 0, con_default->height - offs * pitchy - pitchy, con_default->lines[i]);
		}
		
		/* offset the input line if it's wider than the screen can show. we also keep a buffer of 3 characters at the "other side" of it, so that inplace editing is a bit more sensible. */
		/* todo: blatantly should be able to move the character "within" the buffer within a current line when offset, and only scroll when at the left- or right- edges. */
		/*       should probably do this with a "current line display window" type concept. */
	
		offs = 0;
		cursor_screen_offs = con_default->cursor_offs;
		if (pitchx * con_default->cursor_offs > con_default->width) {
			offs = con_default->cursor_offs - (con_default->width / pitchx) + 3;
			offs = offs < 0 ? 0 : offs;
			cursor_screen_offs = (con_default->width / pitchx) - 3;
		}
		font_print(con_default->font, 0, con_default->height - pitchy, &con_default->input_line[offs]);
		
		
		if (SDL_GetTicks() % 1000 < 500) {
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);
			glColor4f(1,1,1,1);
			glVertex2i(cursor_screen_offs * pitchx, con_default->height);
			glVertex2i(cursor_screen_offs * pitchx + pitchx, con_default->height);
			glVertex2i(cursor_screen_offs * pitchx + pitchx, con_default->height - 2);
			glVertex2i(cursor_screen_offs * pitchx, con_default->height - 2);
			glEnd();
			glEnable(GL_TEXTURE_2D);
		}
		
	}
	
}
/*
	font_printf(world->font, 8, 4, "A madman strikes again! (%.0f fps)",
			renderer_fps(r));
	font_printf(world->font, 8, 24, "x: %.3f y: %.3f", cpos[0], cpos[2]);
*/