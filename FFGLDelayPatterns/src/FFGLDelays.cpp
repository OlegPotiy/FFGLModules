#include "FFGLDelays.h"

static CFFGLPluginInfo PluginInfo(
	FFGLDelays::CreateInstance,	// Create method
	"DL23",						// Plugin unique ID
	"FFGLD3lay",				// Plugin name											
	1,						   	// API major version number 											
	000,						// API minor version number	
	1,							// Plugin major version number
	000,						// Plugin minor version number
	FF_EFFECT,					// Plugin type
	"FFGL D3lay",				// Plugin description
	"by Oleg Potiy"				// About
	);

FFGLDelays::FFGLDelays() :CFreeFrameGLPlugin()
{
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);
}

FFGLDelays::~FFGLDelays()
{

}

const int FB_COUNT = 3;

DWORD	FFGLDelays::InitGL(const FFGLViewportStruct *vp)
{
	glExts.Initialize();

	for (int i = 0; i < FB_COUNT; i++)
	{
		auto fb = FFGLUtils::CreateFrameBuffer(vp->width, vp->height, glExts);
		fbos.push_back(fb);
	};

	return FF_SUCCESS;
}

DWORD	FFGLDelays::DeInitGL()
{
	for (auto fb : fbos)
	{
		FFGLUtils::DeleteFrameBuffer(fb, glExts);
	};

	fbos.clear();

	return FF_SUCCESS;
}

DWORD	FFGLDelays::ProcessOpenGL(ProcessOpenGLStruct* pGL)
{
	return FF_SUCCESS;
}

DWORD	FFGLDelays::SetParameter(const SetParameterStruct* pParam)
{
	return FF_SUCCESS;
}

DWORD	FFGLDelays::GetParameter(DWORD dwIndex)
{
	return FF_SUCCESS;
}

