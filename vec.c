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
