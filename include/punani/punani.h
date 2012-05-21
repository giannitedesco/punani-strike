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

typedef float vec2_t[2];
typedef float vec3_t[3];
typedef float vec4_t[4];
typedef float mat4_t[4][4];
typedef float mat3_t[3][3];

typedef struct prect {
	int x, y;
	int h, w;
}prect_t;

struct AABB_Sweep {
	vec3_t mins;
	vec3_t maxs;
	vec3_t a;
	vec3_t b;
};

struct obb {
	vec3_t origin;
	vec3_t dim; /* extents */
	mat3_t rot; /* othornormal rotation basis */
	vec3_t vel; /* velocity */
};

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

static inline float f_min(float a, float b)
{
	return (a < b) ? a : b;
}

static inline float f_max(float a, float b)
{
	return (a > b) ? a : b;
}

__attribute__((format(printf,1,2))) void con_printf(const char *fmt, ...);

#endif /* _PUNANI_H */
