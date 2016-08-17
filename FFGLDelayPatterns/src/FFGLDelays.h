#pragma once

#include "../../FFGL/FFGLShader.h"
#include "../../FFGL/FFGLPluginSDK.h"
#include "../../FFGL/FFGLLib.h"
#include "../../FFGL/FFGLUtils.h"

#include <string>
#include <math.h>
#include <map>
#include <memory>
#include <vector>

class FFGLDelays : public CFreeFrameGLPlugin
{
private:
	std::vector<FFGLFrameBuffer> fbos;
	FFGLExtensions glExts;

public:

	FFGLDelays();
	~FFGLDelays();

	DWORD	SetParameter(const SetParameterStruct* pParam);
	DWORD	GetParameter(DWORD dwIndex);
	DWORD	ProcessOpenGL(ProcessOpenGLStruct* pGL);
	DWORD	InitGL(const FFGLViewportStruct *vp);
	DWORD	DeInitGL();

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance)
	{
		*ppInstance = new FFGLDelays();
		if (*ppInstance != NULL) return FF_SUCCESS;
		return FF_FAIL;
	}
};
