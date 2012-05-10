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

void con_init(void);
void con_init_display(font_t font, texture_t conback);
int con_keypress(int key, int down, const SDL_KeyboardEvent event);
void con_render(renderer_t r);
void con_free(void);

#endif
