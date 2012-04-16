/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _VEC_H
#define _VEC_H

#include <math.h>

#define X 0
#define Y 1
#define Z 2

static inline float v_len(const vec3_t v)
{
	float len;

	len = (v[X] * v[X]) +
		(v[Y] * v[Y]) +
		(v[Z] * v[Z]);

	return sqrt(len);
}

static inline void  v_scale(vec3_t v, const float s)
{
	v[X] *= s;
	v[Y] *= s;
	v[Z] *= s;
}

static inline void v_normalize(vec3_t v)
{
	float len = v_len(v);

	if ( len == 0.0f )
		return;

	len = 1 / len;
	v[X] *= len;
	v[Y] *= len;
	v[Z] *= len;
}

static inline void v_sub(vec3_t d, const vec3_t v1, const vec3_t v2)
{
	d[X]= v1[X] - v2[X];
	d[Y]= v1[Y] - v2[Y];
	d[Z]= v1[Z] - v2[Z];
}

static inline void v_cross_product(vec3_t d, const vec3_t v1, const vec3_t v2)
{
	d[X] = (v1[Y] * v2[Z]) - (v1[Z] * v2[Y]);
	d[Y] = (v1[Z] * v2[X]) - (v1[X] * v2[Z]);
	d[Z] = (v1[X] * v2[Y]) - (v1[Y] * v2[X]);
}

static inline float v_dot_product(const vec3_t v1, const vec3_t v2)
{
	return (v1[X] * v2[X] + v1[Y] * v2[Y] + v1[Z] * v2[Z]);
}

static inline void mat4_mult_point(vec3_t c, mat4_t m, vec3_t a)
{
	c[0] = m[0][0] * a[0] + m[1][0] * a[1] + m[2][0] * a[2] + m[3][0];
	c[1] = m[0][1] * a[0] + m[1][1] * a[1] + m[2][1] * a[2] + m[3][1];
	c[2] = m[0][2] * a[0] + m[1][2] * a[1] + m[2][2] * a[2] + m[3][2];
}

#endif /* _VEC_H */
