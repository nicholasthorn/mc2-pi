#ifndef GRAPHICS_H
#define GRAPHICS_H

// Single entry point for the OpenGL API.
// MC2_GLES: OpenGL ES 3.1 (e.g. Raspberry Pi 4 / V3D), functions linked
// directly from libGLESv2. Otherwise desktop GL loaded through GLEW.
#ifdef MC2_GLES
	#include <GLES3/gl31.h>
	#include <GLES2/gl2ext.h>

	// Desktop GL names used by shared enum tables. Equivalent ES/extension
	// values where one exists; otherwise desktop values that are never passed
	// to GL on this path.
	#define GL_FUNC_ADD_EXT					GL_FUNC_ADD
	#define GL_FUNC_SUBTRACT_EXT			GL_FUNC_SUBTRACT
	#define GL_FUNC_REVERSE_SUBTRACT_EXT	GL_FUNC_REVERSE_SUBTRACT
	#define GL_DYNAMIC_DRAW_ARB				GL_DYNAMIC_DRAW
	#define GL_STACK_OVERFLOW				GL_STACK_OVERFLOW_KHR
	#define GL_STACK_UNDERFLOW				GL_STACK_UNDERFLOW_KHR
	#define GL_TESS_CONTROL_SHADER			GL_TESS_CONTROL_SHADER_EXT
	#define GL_TESS_EVALUATION_SHADER		GL_TESS_EVALUATION_SHADER_EXT
	#define GL_GEOMETRY_SHADER				GL_GEOMETRY_SHADER_EXT
	#define GL_TEXTURE_CUBE_MAP_ARRAY		GL_TEXTURE_CUBE_MAP_ARRAY_EXT
	#define GL_CLAMP						GL_CLAMP_TO_EDGE
	#define GL_TEXTURE_1D					0x0DE0
	#define GL_TEXTURE_1D_ARRAY				0x8C18
	#define GL_SAMPLER_1D					0x8B5D
	#define GL_QUADS						0x0007

	// No polygon modes in ES: wireframe debug drawing falls back to filled.
	#define GL_LINE							0x1B01
	#define GL_FILL							0x1B02
	inline void glPolygonMode(GLenum, GLenum) {}
#else
	#include <GL/glew.h>
#ifndef __APPLE__
	// macOS has no GL/gl.h (it is OpenGL/gl3.h there); glew.h declares everything we use
	#include <GL/gl.h>
#endif
#endif

#ifndef PLATFORM_WINDOWS
	// because APIENTRY is undef'ed at the end of glew.h and we use it in shader_builder.h
	#ifndef APIENTRY
		#define APIENTRY
	#endif
#else
	#include<windows.h>
#endif



#endif //GRAPHICS_H
