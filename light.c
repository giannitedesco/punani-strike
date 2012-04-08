/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/light.h>

#include <GL/gl.h>
#include <math.h>

struct _light {
	float pos[4];
	vec3_t color;
};

light_t light_new(void)
{
	struct _light *l;

	l = calloc(1, sizeof(*l));
	if ( NULL == l )
		goto out;

	l->pos[0] = 0.0;
	l->pos[1] = 33.0;
	l->pos[2] = -25.0;
	l->pos[3] = 1.0;

	l->color[0] = 1.0;
	l->color[1] = 0.8;
	l->color[2] = 0.6;
out:
	return l;
}

void light_render(light_t l)
{
	glLightfv(GL_LIGHT0, GL_AMBIENT, l->color);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l->color);
	glLightfv(GL_LIGHT0, GL_POSITION, l->pos);

	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glPushMatrix();

	/* Draw an arrowhead. */
	glColor3fv(l->color);
	glTranslatef(l->pos[0], l->pos[1], l->pos[2]);
	glRotatef(-180.0 / M_PI, 0, 1, 0);
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0, 0, 0);
	glVertex3f(2, 1, 1);
	glVertex3f(2, -1, 1);
	glVertex3f(2, -1, -1);
	glVertex3f(2, 1, -1);
	glVertex3f(2, 1, 1);
	glEnd();

	/* Draw a white line from light direction. */
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(5, 0, 0);
	glEnd();

	glPopMatrix();
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
}

void light_free(light_t l)
{
	if ( l ) {
		free(l);
	}
}
