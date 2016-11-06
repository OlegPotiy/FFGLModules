#include "FFGLDelays.h"



static CFFGLPluginInfo PluginInfo(
	FFGLDelays::CreateInstance,	// Create method
	"DL23",						// Plugin unique ID
	"DeeLay",				// Plugin name											
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

	std::map<int, FFGLParameter> lDefs
	{
		{ (int)ParamCodes::NUM_MODULES, FFGLParameter{ "Modules num", 0.5f } },
		{ (int)ParamCodes::PATTERN_MODE , FFGLParameter{ "Pattern mode", 0.0f } },
		{ (int)ParamCodes::CONFIG_BUFFERS_NUMBER , FFGLParameter{ "Buffers count", "120" } }
	};
	//
	parameterDefinitions.insert(lDefs.begin(), lDefs.end());
	//
	for (auto param : parameterDefinitions)
	{
		auto value = param.second;
		if (value.getType() == FF_TYPE_STANDARD)
			SetParamInfo(param.first, value.getName().c_str(), value.getType(), value.getFloatStorage());
		else if (value.getType() == FF_TYPE_TEXT)
			SetParamInfo(param.first, value.getName().c_str(), value.getType(), value.getStrStorage().c_str());
	}

	lDefs.clear();
}

FFGLDelays::~FFGLDelays()
{
	parameterDefinitions.clear();
	DeInitGL();
}




DWORD	FFGLDelays::InitGL(const FFGLViewportStruct *vp)
{
	viewPoint = vp;
	bufferCount = std::stoi(bufferNumber);

	glExts.Initialize();
	reConstructBuffers(viewPoint, bufferCount);

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

void FFGLDelays::processParamsValuesChanges()
{
	auto numModulesParam = parameterDefinitions.find((int)ParamCodes::NUM_MODULES);
	modulesAmountCoeff = numModulesParam->second.getFloatStorage();

	auto patternModesParam = parameterDefinitions.find((int)ParamCodes::PATTERN_MODE );
	patternCoeff = patternModesParam->second.getFloatStorage();

	auto bufferNumberParam = parameterDefinitions.find((int)ParamCodes::CONFIG_BUFFERS_NUMBER);
	bufferNumber.assign(bufferNumberParam->second.getStrStorage());

	if (bufferNumberParam->second.getChanged())
	{
		bufferCount = std::stoi(bufferNumber);
		reConstructBuffers(viewPoint, bufferCount);
	}
	
}


void FFGLDelays::reConstructBuffers(const FFGLViewportStruct *vp, int puffers)
{
	index = 0;

	//Deconstruct buffers first
	DeInitGL();

	
	for (int i = 0; i < puffers; i++)
	{
		auto fb = FFGLUtils::CreateFrameBuffer(viewPoint->width, viewPoint->height, glExts);
		fbos.push_back(fb);
	};
}

DWORD FFGLDelays::ProcessOpenGL(ProcessOpenGLStruct* pGL)
{

	processParamsValuesChanges();

	if (pGL->numInputTextures < 1)
		return FF_FAIL;
	if (pGL->inputTextures[0] == NULL)
		return FF_FAIL;

	FFGLTextureStruct& Texture = *(pGL->inputTextures[0]);
	FFGLTexCoords maxCoords = GetMaxGLTexCoords(Texture);

	glEnable(GL_TEXTURE_2D);


	// saving current frame buffer object
	GLint currentFb;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFb);

	// render to the i-frame buffer	
	// set rendering destination to FBO
	glExts.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbos[index].bufferId);
	glBindTexture(GL_TEXTURE_2D, Texture.Handle);
	glClear(GL_COLOR_BUFFER_BIT);

	FFGLUtils::FrameRect(maxCoords);

	//unbind the input texture
	glBindTexture(GL_TEXTURE_2D, 0);

	// back to the current frame buffer
	glExts.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFb);


	const int moduleCount{ (int)(2.0f * (1.0 - modulesAmountCoeff) + modulesAmountCoeff * ((float)bufferCount)) };
	const double stripScale{ ((double)2) / ((double)moduleCount) };
	const double stripTexScale{ ((double)1) / ((double)moduleCount) };
	const float fStep {((float)bufferCount) / ((float)(moduleCount - 1))};

	float fIndex = GetOldest();

	double geomPos = -1;
	double texPos = 0;

	for (int i = 0; i < moduleCount; i++)
	{
		int fbIndex = (i == 0) ? GetOldest() : (i == (moduleCount - 1)) ? GetNewest() : (int)(GetOldest() + ((float)i) * fStep) % bufferCount;

		glBindTexture(GL_TEXTURE_2D, fbos[fbIndex].textureId);
		if (patternCoeff < 0.5)
			VStrip(geomPos, stripScale, texPos, stripTexScale);
		else
			HStrip(geomPos, stripScale, texPos, stripTexScale);

		geomPos += stripScale;
		texPos += stripTexScale;
	};

	IncIndex();
	
	glDisable(GL_TEXTURE_2D);

	return FF_SUCCESS;
}

DWORD	FFGLDelays::SetParameter(const SetParameterStruct* pParam)
{
	if (pParam != NULL)
	{
		auto result = parameterDefinitions.find(pParam->ParameterNumber);
		if (result != parameterDefinitions.end())
		{
			if (result->second.getType() == FF_TYPE_STANDARD)
			{				
				result->second.setFloatStorage(*((float *)(unsigned)&(pParam->NewParameterValue)));
			}
			else if (result->second.getType() == FF_TYPE_TEXT)
			{
				result->second.setStrStorage((char*)pParam->NewParameterValue);
			}
		}
		else
		{
			return FF_FAIL;
		}

		return FF_SUCCESS;
	}

	return FF_FAIL;
}

DWORD	FFGLDelays::GetParameter(DWORD dwIndex)
{
	auto result = parameterDefinitions.find(dwIndex);
	if (result != parameterDefinitions.end())
	{
		if (result->second.getType() == FF_TYPE_STANDARD)
		{
			DWORD dwRet;
			*((float *)(unsigned)(&dwRet)) = result->second.getFloatStorage();
			return dwRet;
		}
		else if (result->second.getType() == FF_TYPE_TEXT)
		{
			DWORD dwRet;
			*((const char **)(unsigned)(&dwRet)) = result->second.getStrStorage().c_str();
			return dwRet;
		}
	}

	return FF_FAIL;
}



static void VStrip(double tPos, double height, double lTexPos, double texWidth)
{
	glBegin(GL_QUADS);

	//lower left
	glTexCoord2d(0.0, lTexPos);
	glVertex2f(-1, tPos);

	//upper left	
	glTexCoord2d(0.0, lTexPos + texWidth);
	glVertex2f(-1, tPos + height);

	//upper right	
	glTexCoord2d(1.0, lTexPos + texWidth);
	glVertex2f(1, tPos + height);

	//lower right
	glTexCoord2d(1.0, lTexPos);
	glVertex2f(1, tPos);

	glEnd();

}

static void HStrip(double lPos, double width, double lTexPos, double texWidth)
{
	glBegin(GL_QUADS);

	//lower left
	glTexCoord2d(lTexPos, 0.0);
	glVertex2f(lPos, -1);

	//upper left	
	glTexCoord2d(lTexPos, 1);
	glVertex2f(lPos, 1);

	//upper right	
	glTexCoord2d(lTexPos + texWidth, 1);
	glVertex2f(lPos + width, 1);

	//lower right
	glTexCoord2d(lTexPos + texWidth, 0.0);
	glVertex2f(lPos + width, -1);

	glEnd();
}