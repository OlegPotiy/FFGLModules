#pragma once

#include "../../FFGL/FFGLShader.h"
#include "../../FFGL/FFGLPluginSDK.h"
#include "../../FFGL/FFGLLib.h"
#include "../../FFGL/FFGLUtils.h"
#include "../../FFGL/utils/FFGLParameter.h"

#include <string>
#include <math.h>
#include <map>
#include <memory>
#include <vector>


enum class ParamCodes : int
{
	NUM_MODULES,
	PATTERN_MODE,
	CONFIG_BUFFERS_NUMBER
};



static void HStrip(double lPos, double width, double lTexPos, double texWidth);
static void VStrip(double lPos, double width, double lTexPos, double texWidth);

class FFGLDelays : public CFreeFrameGLPlugin
{
private:

	
	std::map<int, FFGLParameter> parameterDefinitions;
	std::vector<FFGLFrameBuffer> fbos;
	FFGLExtensions glExts;

	int bufferCount{ 121 };
	int index{ 0 };

	std::string bufferNumber{ "120" };
	GLfloat modulesAmountCoeff{ 1.0f };
	GLfloat patternCoeff{ 1.0f };

	const FFGLViewportStruct *viewPoint;

	inline int GetOldest() { return (index + 1) % bufferCount; }
	inline int GetNewest() { return index; }

	inline int GetIndex() { return GetNewest(); }
	inline int IncIndex() { index = GetOldest(); return index; }

	void processParamsValuesChanges();
	void reConstructBuffers(const FFGLViewportStruct *vp, int bufferCount);


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
