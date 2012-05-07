/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_FONT_H
#define _PUNANI_FONT_H

typedef struct _font *font_t;

font_t font_load(renderer_t r, const char *fn, float px, float py);
void font_print(font_t f, unsigned int x, unsigned int y, const char *str);
__attribute__((format(printf,4,5)))
void font_printf(font_t f, unsigned int x, unsigned int y, const char *fmt, ...);
void font_free(font_t f);

#endif /* _PUNANI_FONT_H */
