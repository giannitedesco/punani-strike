/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/blob.h>
#include <punani/punani_gl.h>
#include <punani/shader.h>

#define SHADER_MAX_VERT		4
#define SHADER_MAX_FRAG		4
struct _shader {
	GLuint s_prog;
	GLuint s_vert[SHADER_MAX_VERT];
	GLuint s_frag[SHADER_MAX_FRAG];
	unsigned int s_num_vert;
	unsigned int s_num_frag;
};

static const char *get_shader_infolog(GLuint s)
{
	static GLchar buf[512];
	GLsizei len;

	glGetShaderInfoLog(s, sizeof(buf) - 1, &len, buf);
	buf[len] = '\0';
	return buf;
}

static const char *get_prog_infolog(struct _shader *s)
{
	static GLchar buf[512];
	GLsizei len;

	glGetProgramInfoLog(s->s_prog, sizeof(buf) - 1, &len, buf);
	buf[len] = '\0';
	return buf;
}

static int compile_shader(const char *fn, GLint type, GLuint *id)
{
	uint8_t *ptr;
	size_t len;
	const GLchar *gpt;
	GLint gsz;
	GLint compiled;
	GLint s;

	ptr = blob_from_file(fn, &len);
	if ( NULL == ptr )
		goto err;

	s = glCreateShader(type);
	if ( !s )
		goto err_free;

	gpt = (GLchar *)ptr;
	gsz = len;
	glShaderSource(s, 1, &gpt, &gsz);

	glCompileShader(s);

	glGetShaderiv(s, GL_COMPILE_STATUS, &compiled);
	if ( !compiled ) {
		con_printf("compile_shader: %s: %s\n", fn, get_shader_infolog(s));
		goto err_del;
	}

	*id = s;
	blob_free(ptr, len);
	return 1;

err_del:
	glDeleteShader(s);
err_free:
	blob_free(ptr, len);
err:
	return 0;
}

shader_t shader_new(void)
{
	struct _shader *s;

	s = calloc(1, sizeof(*s));
	if ( NULL == s )
		goto out;

	s->s_prog = glCreateProgram();
	if ( !s->s_prog )
		goto out_free;

	/* success */
	goto out;

out_free:
	free(s);
	s = NULL;
out:
	return s;
}

int shader_add_vert(shader_t s, const char *fn)
{
	if ( s->s_num_vert >= SHADER_MAX_VERT )
		return 0;

	if ( !compile_shader(fn, GL_VERTEX_SHADER,
				&s->s_vert[s->s_num_vert]) )
		return 0;

	glAttachShader(s->s_prog, s->s_vert[s->s_num_vert]);
	s->s_num_vert++;
	return 1;
}

int shader_add_frag(shader_t s, const char *fn)
{
	if ( s->s_num_frag >= SHADER_MAX_FRAG )
		return 0;

	if ( !compile_shader(fn, GL_FRAGMENT_SHADER,
				&s->s_frag[s->s_num_frag]) )
		return 0;

	glAttachShader(s->s_prog, s->s_frag[s->s_num_frag]);
	s->s_num_frag++;
	return 1;
}

int shader_link(shader_t s)
{
	GLint linked;

	glLinkProgram(s->s_prog);
	glGetProgramiv(s->s_prog, GL_LINK_STATUS, &linked);
	if ( !linked ) {
		con_printf("shader_link: %s\n", get_prog_infolog(s));
		return 0;
	}
	return 1;
}

void shader_free(shader_t s)
{
	if ( s ) {
		glDeleteProgram(s->s_prog);
		free(s);
	}
}

int shader_uniform_float(shader_t s, const char *name, float f)
{
	GLint loc;
	GLenum err;

	loc = glGetUniformLocation(s->s_prog, name);
	if ( loc < 0 ) {
		con_printf("shader: uniform: %s not found\n", name);
		return 0;
	}

	glUseProgram(s->s_prog);
	glUniform1f(loc, f);
	err = glGetError();
	glUseProgram(0);
	return (err == GL_NO_ERROR);
}

int shader_uniform_int(shader_t s, const char *name, int i)
{
	GLint loc;
	GLenum err;

	loc = glGetUniformLocation(s->s_prog, name);
	if ( loc < 0 ) {
		con_printf("shader: uniform: %s not found\n", name);
		return 0;
	}

	glUseProgram(s->s_prog);
	glUniform1i(loc, i);
	err = glGetError();
	glUseProgram(0);
	return (err == GL_NO_ERROR);
}

void shader_begin(shader_t s)
{
	glUseProgram(s->s_prog);
}

void shader_end(shader_t s)
{
	glUseProgram(0);
}
