/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_GL_H
#define _PUNANI_GL_H

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>

#ifndef APIENTRYP 
#define APIENTRYP APIENTRY *
#endif

#ifndef PFNGLACTIVESTENCILFACEEXTPROC
typedef void (APIENTRYP PFNGLACTIVESTENCILFACEEXTPROC) (GLenum face);
#endif

#ifndef GL_STENCIL_TEST_TWO_SIDE_EXT
#define GL_STENCIL_TEST_TWO_SIDE_EXT 0x8910
#endif

#endif

#endif /* _PUNANI_GL_H */
