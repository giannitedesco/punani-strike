#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <SDL.h>
#include <punani/punani.h>
#include <punani/punani_gl.h>
#include <punani/console.h>
#include <punani/cvar.h>

/* max line len including null terminator */
#define CONSOLE_LINE_MAX_LEN 1024
#define CONSOLE_MAX_LINES 100
#define CONSOLE_HISTORY_SIZE 30

#define CONSOLE_VISIBLE 1
/* todo: #define CONSOLE_MOVING :) */
#define CONSOLE_HIDDEN 16

static void con_do_input(char *input);

static unsigned int con_display_lines = 10;

struct _console {
	/* one of CONSOLE_VISIBLE or CONSOLE_HIDDEN */
	int  state;

	font_t font;
	texture_t conback;

	/* console log stuff */
	char lines[CONSOLE_MAX_LINES][CONSOLE_LINE_MAX_LEN];
	int  line;
	int  lines_size;
	
	/* line input stuff */
	int  cursor_offs;
	char input_line[CONSOLE_LINE_MAX_LEN];
	
	/* input history */
	char history[CONSOLE_HISTORY_SIZE][CONSOLE_LINE_MAX_LEN];
	int  history_pos;
};

console_t con_default = NULL;

void con_init(void)
{
	con_default = calloc(1, sizeof(*con_default));
	con_default->state = CONSOLE_HIDDEN;
	con_printf("punani strike console\n");
}

void con_init_display(font_t font, texture_t conback)
{
	font_free(con_default->font);
	texture_put(con_default->conback);

	con_default->font = font;
	con_default->conback = conback;
	if (con_default->conback) {
		con_printf("using conback (%d x %d)\n", texture_height(con_default->conback), texture_width(con_default->conback));
	}
	
	cvar_register_uint("console", "lines", &con_display_lines);
}

void con_printf(const char *fmt, ...)
{
	va_list args;

	if ( NULL == con_default ) {
		return;
	}
		
	char buf[CONSOLE_LINE_MAX_LEN];
	char *ptr;
	char *nl;
	int line_len;
	
	va_start(args, fmt);
	line_len = vsnprintf(buf, CONSOLE_LINE_MAX_LEN, fmt, args);
	va_end(args);
	
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	
	/* if this line is gonna exceed our buffer, and it was "expecting" to be newline terminated, force a newline in there as well.
	 * this still isn't perfect for some situations, like fmt = "%s\n%s", but it's close enough. */
	if (line_len >= CONSOLE_LINE_MAX_LEN && fmt[strlen(fmt) - 1] == '\n') {
		buf[CONSOLE_LINE_MAX_LEN - 2] = '\n';
	}
	
	ptr = buf;
	nl = strchr(ptr, '\n');

	while( NULL != nl ) {
		*nl = '\0';
		snprintf(con_default->lines[con_default->line], CONSOLE_LINE_MAX_LEN, "%s%s", con_default->lines[con_default->line], ptr);
		con_default->line = (con_default->line + 1) % CONSOLE_MAX_LINES;
		con_default->lines_size++;
		con_default->lines[con_default->line][0] = '\0';
		ptr = nl + 1;
		nl = strchr(ptr, '\n');
	}
	
	snprintf(con_default->lines[con_default->line], CONSOLE_LINE_MAX_LEN, "%s%s", con_default->lines[con_default->line], ptr);
}

/* splice some characters out of the current input line. current input line ends up consisting of line[s0_start]..line[s0_end] + line[s1_start]..line[CONSOLE_LINE_MAX_LEN]. */
static void con_input_splice(int s0_start, int s0_end, int s1_start)
{
	assert(s0_end < CONSOLE_LINE_MAX_LEN);
	assert(s0_start < CONSOLE_LINE_MAX_LEN);
	
	char newline[CONSOLE_LINE_MAX_LEN];
	int s0_len = s0_end - s0_start;
	
	strncpy(newline, &con_default->input_line[s0_start], s0_len);
	strncpy(&newline[s0_len], &con_default->input_line[s1_start], CONSOLE_LINE_MAX_LEN - s1_start);
	
	strncpy(con_default->input_line, newline, CONSOLE_LINE_MAX_LEN);
}

/* add the typed char into the string where the cursor is. */
static void con_input_insert(char c)
{
	if (con_default->cursor_offs == CONSOLE_LINE_MAX_LEN) {
		return;
	}
	
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

static void con_history_to_input(int dir)
{
	con_default->history_pos += dir;
	
	if (con_default->history_pos < 0) {
		con_default->history_pos += CONSOLE_HISTORY_SIZE;
	}
	con_default->history_pos = con_default->history_pos % CONSOLE_HISTORY_SIZE;
	
	strncpy(con_default->input_line, con_default->history[con_default->history_pos], CONSOLE_LINE_MAX_LEN);
	con_default->cursor_offs = strlen(con_default->input_line);
}

int con_keypress(int key, int down, const SDL_KeyboardEvent event)
{
	if ( NULL == con_default ) {
		return 0;
	}
	
	/* console hide/show logic for backquote key */
	if ( SDLK_BACKQUOTE == key ) {
		if ( down && CONSOLE_HIDDEN == con_default->state ) {
			con_default->state = CONSOLE_VISIBLE;
			SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		} else if (down) {
			con_default->state = CONSOLE_HIDDEN;
			SDL_EnableKeyRepeat(0, 0);
		}
		return 1;
	}

	if ( CONSOLE_VISIBLE == con_default->state && down ) {
		
		/* todo: capture the key, do the line input, all that jazz */
		switch(event.keysym.sym) {
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
			if (event.keysym.mod & KMOD_CTRL) {
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
						/* just default to the start of the line. */
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
			if (event.keysym.mod & KMOD_CTRL) {
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
		case SDLK_UP:
			con_history_to_input(-1);
			break;
		case SDLK_DOWN:
			con_history_to_input(1);
			break;
		case SDLK_HOME:
			con_default->cursor_offs = 0;
			break;
		case SDLK_END:
			con_default->cursor_offs = strlen(con_default->input_line);
			break;
		case SDLK_RETURN:
			con_printf("%s\n", con_default->input_line);
			con_do_input(con_default->input_line);
			con_default->cursor_offs = 0;
			con_default->input_line[0] = '\0';
			break;
		default:
			if (key >= 0x20 && key <= 0x7F) {
				con_input_insert((char)event.keysym.unicode);
			}
		}
		
		return 1;
	} else if ( CONSOLE_VISIBLE == con_default->state && !down ) {
		/* we only hide the console on escape key up, to prevent the keyup propagating into the game and quitting it. */
		if (key == SDLK_ESCAPE) {
			con_default->state = CONSOLE_HIDDEN;
		}
		
		return 1;
	}
	
	return 0;
}

/* con_do_input will mangle your input string, so plz don't rely on it after passing it here. */
static void con_do_input(char *input) {
	/* push the input history first. */
	strncpy(con_default->history[con_default->history_pos], input, CONSOLE_LINE_MAX_LEN);
	con_default->history_pos = (con_default->history_pos + 1) % CONSOLE_HISTORY_SIZE;
	
	cvar_con_input(input);
}


void con_render(renderer_t r)
{
	unsigned int width, height;

	if ( NULL == con_default )
		return;

	renderer_size(r, &width, &height);

	if ( CONSOLE_HIDDEN == con_default->state) {
		/* todo: draw the "last 4 lines" */
	} else {
		float pitchx, pitchy;
		int i, offs, cursor_screen_offs;

		font_get_pitch(con_default->font, &pitchx, &pitchy);
		
		int const num_lines = con_display_lines;
		int const visible_height = pitchy * (num_lines + 1);
		int const border_top = height - visible_height - 5;
		
		if (con_default->conback) {
			glEnable(GL_TEXTURE_2D);
			texture_bind(con_default->conback);
			/* try toggling disabling blend - looks nice in different ways in each setting */
			/* glDisable(GL_BLEND); */
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glBegin(GL_QUADS);
			glColor4f(1,1,1,1);
			glTexCoord2f(0, 0); glVertex2i(0, border_top);
			glTexCoord2f(1, 0); glVertex2i(width, border_top);
			glTexCoord2f(1, 1); glVertex2i(width, height);
			glTexCoord2f(0, 1); glVertex2i(0, height);
			glEnd();
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);
			glColor4f(0, 1.f, 0, 0.f);
			glVertex2i(0, border_top);
			glVertex2i(width, border_top);
			glVertex2i(width, height);
			glVertex2i(0, height);
			glEnd();
			glEnable(GL_TEXTURE_2D);
		}

		offs = (con_default->lines_size % CONSOLE_MAX_LINES) - num_lines;
		for(i = 0; i < num_lines; i++, offs++) {
			if (offs < 0 && con_default->lines_size < CONSOLE_MAX_LINES) {
				/* well, this line hasn't actually been typed, so do nothing. */
			} else {
				if (offs < 0) offs += CONSOLE_MAX_LINES;
				font_print(con_default->font, 0, border_top + i * pitchy, con_default->lines[offs % CONSOLE_MAX_LINES]);
			}
		}

		/* offset the input line if it's wider than the screen can show. we also keep a buffer of 3 characters at the "other side" of it, so that inplace editing is a bit more sensible. */
		/* todo: blatantly should be able to move the character "within" the buffer within a current line when offset, and only scroll when at the left- or right- edges. */
		/*       should probably do this with a "current line display window" type concept. */

		offs = 0;
		cursor_screen_offs = con_default->cursor_offs;
		if (pitchx * con_default->cursor_offs > width) {
			offs = con_default->cursor_offs - (width / pitchx) + 3;
			offs = offs < 0 ? 0 : offs;
			cursor_screen_offs = (width / pitchx) - 3;
		}
		font_print(con_default->font, 0, height - pitchy, &con_default->input_line[offs]);

		/* flashing cursor */
		if (SDL_GetTicks() % 1000 < 500) {
			glBegin(GL_QUADS);
			glColor4f(1,1,1,1);
			glVertex2i(cursor_screen_offs * pitchx, height);
			glVertex2i(cursor_screen_offs * pitchx + pitchx, height);
			glVertex2i(cursor_screen_offs * pitchx + pitchx, height - 2);
			glVertex2i(cursor_screen_offs * pitchx, height - 2);
			glEnd();
		}

	}

}
