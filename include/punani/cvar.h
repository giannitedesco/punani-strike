/*
 * This file is part of punani strike
 * Copyright (c) 2012 Steven Dickinson
 * Released under the terms of the GNU GPL version 2
 */
#ifndef _PS_CVARS_H_
#define _PS_CVARS_H_

typedef struct _cvar *cvar_t;

cvar_t cvar_locate(const char *ns, const char *name);
void cvar_register_float(const char *namespace, const char *cvar, float *value);
void cvar_register_uint(const char *ns, const char *name, unsigned int *ptr);
void cvar_set(cvar_t cvar, const char *value);
void cvar_con_input(char *input);
void cvar_load(const char *filename);
void cvar_save(const char *filename);

#endif
