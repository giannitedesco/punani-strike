/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_SHADER_H
#define _PUNANI_SHADER_H

typedef struct _shader *shader_t;

shader_t shader_new(void);

int shader_add_vert(shader_t s, const char *fn);
int shader_add_frag(shader_t s, const char *fn);
int shader_link(shader_t s);

int shader_uniform_float(shader_t s, const char *name, float f);
int shader_uniform_int(shader_t s, const char *name, int i);

void shader_begin(shader_t s);
void shader_end(shader_t s);

void shader_free(shader_t s);

#endif /* _PUNANI_SHADER_H */
