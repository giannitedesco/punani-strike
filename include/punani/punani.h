/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_H
#define _PUNANI_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

typedef float vec3_t[3];
typedef float mat4_t[4][4];

typedef struct prect {
	int x, y;
	int h, w;
}prect_t;

static inline int r_min(int a, int b)
{
	return (a < b) ? a : b;
}

static inline int r_max(int a, int b)
{
	return (a > b) ? a : b;
}

static inline int r_clamp(int x, int lbound, int ubound)
{
	return r_max(lbound, r_min(x, ubound));
}

#endif /* _PUNANI_H */
