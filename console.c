#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <SDL.h>
#include <punani/punani.h>
#include <punani/punani_gl.h>
#include <punani/console.h>


#define CONSOLE_LINE_MAX_LEN 1024
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
	char lines[CONSOLE_MAX_LINES][CONSOLE_LINE_MAX_LEN];
	int  line;

    /* line input stuff */
	char input_line[CONSOLE_LINE_MAX_LEN];
	int  cursor_offs;
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
}

int con_keypress(int key, int down) {
	if ( NULL == con_default ) goto out_unused_key;
	if ( !down ) goto out_unused_key;
	
	if ( SDLK_BACKQUOTE == key ) {
		if ( CONSOLE_HIDDEN == con_default->state ) {
			con_default->state = CONSOLE_VISIBLE;
		} else { 
			con_default->state = CONSOLE_HIDDEN;
		}
		goto out_used_key;
	}
	
	if ( CONSOLE_VISIBLE == con_default->state ) {
		
		/* todo: capture the key, do the line input, all that jazz */
		
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
		int const pitch = 16;
		int const num_lines = 10;
		int const visible_height = pitch * num_lines;
		int const border_top = con_default->height - visible_height - 5;
		int i, offs;
		
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
		
		for(i = con_default->line - 1, offs = 1; i >= 0 && offs <= num_lines; i--, offs++) {
			font_print(con_default->font, 0, con_default->height - offs * pitch - pitch, con_default->lines[i]);
		}
	}
	
}
/*
	font_printf(world->font, 8, 4, "A madman strikes again! (%.0f fps)",
			renderer_fps(r));
	font_printf(world->font, 8, 24, "x: %.3f y: %.3f", cpos[0], cpos[2]);
*/