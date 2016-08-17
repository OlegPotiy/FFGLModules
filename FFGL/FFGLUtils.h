#pragma once
#include "FFGL.h"
#include "FFGLExtensions.h"

struct FFGLFrameBuffer
{
	GLuint bufferId;
	GLuint textureId;
	GLenum status;
};

class FFGLUtils
{
public:
	
	static FFGLFrameBuffer CreateFrameBuffer(GLsizei width, GLsizei height, FFGLExtensions& glExts);

	static void DeleteFrameBuffer(FFGLFrameBuffer& fb, FFGLExtensions& glExts);

};

