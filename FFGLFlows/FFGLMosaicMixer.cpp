#include "FFGL/FFGL.h"
#include "FFGL/FFGLLib.h"
#include "FFGLMosaicMixer.h"
#include <gl\GLU.h>
#include <math.h>

// Parameters
#define	FFPARAM_Blend		(0)


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo(
	FFGLMosaicMixer::CreateInstance,	// Create method
	"Mosaic",						// Plugin unique ID
	"MosaicMixer",				// Plugin name											
	1,						   	// API major version number 											
	000,						// API minor version number	
	1,							// Plugin major version number
	000,						// Plugin minor version number
	FF_EFFECT,					// Plugin type
	"FFGL Mosaic",				// Plugin description
	"by Oleg Potiy"				// About
	);




////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FFGLMosaicMixer::FFGLMosaicMixer() :CFreeFrameGLPlugin()
{
	// Input properties
	SetMinInputs(2);
	SetMaxInputs(2);

	// parameters:
	SetParamInfo(FFPARAM_Blend, "Blend", FF_TYPE_STANDARD, 0.5f);
	m_blend = 0.5f;
}

FFGLMosaicMixer::~FFGLMosaicMixer()
{
}

DWORD FFGLMosaicMixer::InitGL(const FFGLViewportStruct *vp)
{
	return FF_SUCCESS;
}

DWORD FFGLMosaicMixer::DeInitGL()
{
	return FF_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD FFGLMosaicMixer::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	FFGLTextureStruct &TextureObject1 = *(pGL->inputTextures[0]);
	FFGLTextureStruct &TextureObject2 = *(pGL->inputTextures[1]);

	DWORD frameHeight = pGL->inputTextures[0]->HardwareHeight;
	DWORD frameWidth = pGL->inputTextures[0]->HardwareWidth;

	glEnable(GL_TEXTURE_2D);



	glDisable(GL_TEXTURE_2D);

	return FF_SUCCESS;
}



DWORD	FFGLMosaicMixer::SetParameter(const SetParameterStruct* pParam)
{
	float fNewValue = 0;

	if (pParam != NULL)
	{
		fNewValue = *((float *)(unsigned)&(pParam->NewParameterValue));

		switch (pParam->ParameterNumber)
		{
		case 0:

			this->m_blend = fNewValue;
			
			break;

		default:
			return FF_FAIL;
		}
	};
	return FF_SUCCESS;
}

DWORD FFGLMosaicMixer::GetParameter(DWORD dwIndex)
{
	DWORD dwReturnValue;

	switch (dwIndex)
	{
	case FFPARAM_Blend:
		*((float*)(unsigned)(&dwReturnValue)) = this->m_blend;
		return dwReturnValue;

	default:
		return FF_FAIL;
	}

	return FF_FAIL;
}