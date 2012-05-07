/*
 * This file is part of punani strike
 * Copyright (c) 2012 Steven Dickinson
 * Released under the terms of the GNU GPL version 2
 */
#ifndef _PS_CONSOLE_H_
#define _PS_CONSOLE_H_

#include <punani/renderer.h>
#include <punani/font.h>
#include <punani/tex.h>


typedef struct _console *console_t;

void con_init(font_t font, texture_t conback, const int screen_width, const int screen_height);
__attribute__((format(printf,1,2)))
void con_printf(const char *fmt, ...);
int con_keypress(int key, int down, const SDL_KeyboardEvent event);
void con_render(void);

#endif
