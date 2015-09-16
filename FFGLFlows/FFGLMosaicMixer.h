#ifndef FFGLBRIGHTNESS_H
#define FFGLBRIGHTNESS_H

#include "FFGL/FFGLShader.h"
#include "FFGL/FFGLPluginSDK.h"


class FFGLMosaicMixer: public CFreeFrameGLPlugin
{

public:
	FFGLMosaicMixer();
	virtual ~FFGLMosaicMixer();

    ///////////////////////////////////////////////////
    // FreeFrame plugin methods
    ///////////////////////////////////////////////////

    DWORD	SetParameter(const SetParameterStruct* pParam);		
    DWORD	GetParameter(DWORD dwIndex);

    DWORD	ProcessOpenGL(ProcessOpenGLStruct* pGL);

	DWORD	InitGL(const FFGLViewportStruct *vp);
	DWORD	DeInitGL();
 


    ///////////////////////////////////////////////////
    // Factory method
    ///////////////////////////////////////////////////

    static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance)
    {
		*ppInstance = new FFGLMosaicMixer();
        if (*ppInstance != NULL) return FF_SUCCESS;
        return FF_FAIL;
    }

private:

	FFGLExtensions m_extensions;
	
	// Parameters
	float m_blend;

	

};



#endif
