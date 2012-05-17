/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/vec.h>

static int intersect(float fDst1, float fDst2,
			   const vec3_t a, const vec3_t b, vec3_t hit)
{
	if ((fDst1 * fDst2) >= 0.0f)
		return 0;
	if (fDst1 == fDst2)
		return 0;

	//v_sub(hit, b, a);
	//v_add(hit, a);
	v_copy(hit, b);
	v_scale(hit, (-fDst1 / (fDst2 - fDst1)));
	return 1;
}

static int in_box(vec3_t hit, const vec3_t mins, const vec3_t maxs,
		 const int Axis)
{
	if (Axis == 1 && hit[2] > mins[2] && hit[2] < maxs[2]
	    && hit[1] > mins[1]
	    && hit[1] < maxs[1])
		return 1;
	if (Axis == 2 && hit[2] > mins[2] && hit[2] < maxs[2]
	    && hit[0] > mins[0]
	    && hit[0] < maxs[0])
		return 1;
	if (Axis == 3 && hit[0] > mins[0] && hit[0] < maxs[0]
	    && hit[1] > mins[1]
	    && hit[1] < maxs[1])
		return 1;
	return 0;
}

int collide_box_line(const vec3_t mins, const vec3_t maxs,
			const vec3_t a, const vec3_t b, vec3_t hit)
{
	/* fast checks, both ends of line on same side of 6 planes */
	if (b[0] < mins[0] && a[0] < mins[0])
		return 0;
	if (b[0] > maxs[0] && a[0] > maxs[0])
		return 0;
	if (b[1] < mins[1] && a[1] < mins[1])
		return 0;
	if (b[1] > maxs[1] && a[1] > maxs[1])
		return 0;
	if (b[2] < mins[2] && a[2] < mins[2])
		return 0;
	if (b[2] > maxs[2] && a[2] > maxs[2])
		return 0;

	/* start inside box */
	if (a[0] > mins[0] && a[0] < maxs[0] &&
	    a[1] > mins[1] && a[1] < maxs[1] &&
	    a[2] > mins[2] && a[2] < maxs[2]) {
		v_copy(hit, a);
		return 1;
	}

	if ((intersect(a[0] - mins[0], b[0] - mins[0], a, b, hit)
	     && in_box(hit, mins, maxs, 1))
	    || (intersect(a[1] - mins[1], b[1] - mins[1], a, b, hit)
		&& in_box(hit, mins, maxs, 2))
	    || (intersect(a[2] - mins[2], b[2] - mins[2], a, b, hit)
		&& in_box(hit, mins, maxs, 3))
	    || (intersect(a[0] - maxs[0], b[0] - maxs[0], a, b, hit)
		&& in_box(hit, mins, maxs, 1))
	    || (intersect(a[1] - maxs[1], b[1] - maxs[1], a, b, hit)
		&& in_box(hit, mins, maxs, 2))
	    || (intersect(a[2] - maxs[2], b[2] - maxs[2], a, b, hit)
		&& in_box(hit, mins, maxs, 3)))
		return 1;

	return 0;
}

#define A(row,col)  (a[row][col])
#define B(row,col)  (b[row][col])
#define P(row,col)  (out[row][col])

/**
 * Perform a full 4x4 matrix multiplication.
 *
 * \param a matrix.
 * \param b matrix.
 * \param product will receive the product of \p a and \p b.
 *
 * \warning Is assumed that \p product != \p b. \p product == \p a is allowed.
 *
 * \note KW: 4*16 = 64 multiplications
 *
 * \author This \c matmul was contributed by Thomas Malik
 */
void mat4_mult(mat4_t out, const mat4_t a, const mat4_t b)
{
   int i;
   for (i = 0; i < 4; i++) {
      const float ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
      P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
      P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
      P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
      P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
   }
}

/**
 * Multiply two matrices known to occupy only the top three rows, such
 * as typical model matrices, and orthogonal matrices.
 *
 * \param a matrix.
 * \param b matrix.
 * \param product will receive the product of \p a and \p b.
 */
void mat3_mult(mat3_t out, const mat3_t a, const mat3_t b)
{
   int i;
   for (i = 0; i < 3; i++) {
      const float ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2);
      P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0);
      P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1);
      P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2);
   }
}

#undef A
#undef B
#undef P

void mat3_load_identity(mat3_t mat)
{
	unsigned int i, j;
	for(i = 0; i < 3; i++) {
		for(j = 0; j < 3; j++) {
			if ( i == j ) {
				mat[i][j] = 1.0;
			}else{
				mat[i][j] = 0.0;
			}
		}
	}
}

void mat4_load_identity(mat3_t mat)
{
	unsigned int i, j;
	for(i = 0; i < 4; i++) {
		for(j = 0; j < 4; j++) {
			if ( i == j ) {
				mat[i][j] = 1.0;
			}else{
				mat[i][j] = 0.0;
			}
		}
	}
}

/* Othonormal basis rotation */
void basis_rotateX(mat3_t mat, float angle)
{
	float s, c;
	mat3_t m;

	if ( angle == 0.0 )
		return;

	mat3_load_identity(m);
	s = sin(angle);
	c = cos(angle);

	m[1][1] = c;
	m[2][2] = c;
	m[1][2] = -s;
	m[2][1] = s;

	mat3_mult(mat, (const float (*)[3])mat, (const float (*)[3])m);
}

void basis_rotateY(mat3_t mat, float angle)
{
	float s, c;
	mat3_t m;

	if ( angle == 0.0 )
		return;

	mat3_load_identity(m);
	s = sin(angle);
	c = cos(angle);

	m[0][0] = c;
	m[2][2] = c;
	m[0][2] = -s;
	m[2][0] = s;

	mat3_mult(mat, (const float (*)[3])mat, (const float (*)[3])m);
}

void basis_rotateZ(mat3_t mat, float angle)
{
	float s, c;
	mat3_t m;

	if ( angle == 0.0 )
		return;

	mat3_load_identity(m);
	s = sin(angle);
	c = cos(angle);

	m[0][0] = c;
	m[1][1] = c;
	m[0][1] = -s;
	m[1][0] = s;

	mat3_mult(mat, (const float (*)[3])mat, (const float (*)[3])m);
}
