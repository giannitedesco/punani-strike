/*
 * This file is part of punani strike
 * Copyright (c) 2012 Steven Dickinson
 * Released under the terms of the GNU GPL version 2
 */
#ifndef _PS_CVARS_H_
#define _PS_CVARS_H_

typedef struct _cvar_ns *cvar_ns_t;
typedef struct _cvar *cvar_t;

#define CVAR_FLAG_SAVE_NOTDEFAULT (1 << 0)
#define CVAR_FLAG_SAVE_ALWAYS     (1 << 1)
#define CVAR_FLAG_SAVE_NEVER      (1 << 2)

cvar_ns_t cvar_ns_new(const char *ns);
void cvar_ns_free(cvar_ns_t ns);

cvar_ns_t cvar_get_ns(const char *name);
cvar_t cvar_locate(cvar_ns_t ns, const char *name);

void cvar_register_float(cvar_ns_t ns, const char *name, int flags, float *ptr);
void cvar_register_uint(cvar_ns_t ns, const char *name, int flags, unsigned int *ptr);

void cvar_set(cvar_ns_t ns, cvar_t cvar, const char *value);
void cvar_con_input(char *input);

void cvar_ns_load(cvar_ns_t ns);
void cvar_ns_save(cvar_ns_t ns);

#endif
