#include "FFGLUtils.h"

FFGLFrameBuffer FFGLUtils::CreateFrameBuffer(GLsizei width, GLsizei height, FFGLExtensions& glExts)
{
	FFGLFrameBuffer fb{ 0, 0, GL_FRAMEBUFFER_COMPLETE_EXT };

	glExts.glGenFramebuffersEXT(1, &fb.bufferId);
	glExts.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb.bufferId);

	glGenTextures(1, &fb.textureId);
	glBindTexture(GL_TEXTURE_2D, fb.textureId);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glExts.glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fb.textureId, 0);

	fb.status = glExts.glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

	return fb;
}

void FFGLUtils::DeleteFrameBuffer(FFGLFrameBuffer& fb, FFGLExtensions& glExts)
{
	if (fb.textureId != 0)	
		glDeleteTextures(1, &fb.textureId);

	if (fb.bufferId != 0)
		glExts.glDeleteFramebuffersEXT(1, &fb.bufferId);		
}

