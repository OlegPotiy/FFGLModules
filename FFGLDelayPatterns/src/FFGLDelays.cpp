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

	auto patternModesParam = parameterDefinitions.find((int)ParamCodes::PATTERN_MODE);
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
	const double stripGeomWidth{ ((double)2) / ((double)moduleCount) };
	const double stripTexWigth{ ((double)1) / ((double)moduleCount) };
	const float fStep{ ((float)bufferCount) / ((float)(moduleCount - 1)) };

	float fIndex = GetOldest();

	double geomPos = -1;
	double texPos = 0;

	auto quadType{ GetQuadType() };
	
	if (quadType == VERTICAL_STRIPS || quadType == HORIZONTAL_STRIPS)
	{
		for (int i = 0; i < moduleCount; i++)
		{
			int fbIndex = (i == 0) ? GetOldest() : (i == (moduleCount - 1)) ? GetNewest() : (int)(GetOldest() + ((float)i) * fStep) % bufferCount;
			glBindTexture(GL_TEXTURE_2D, fbos[fbIndex].textureId);

			switch (quadType)
			{
			case VERTICAL_STRIPS:
				VStrip(geomPos, stripGeomWidth, texPos, stripTexWigth);
				break;
			case HORIZONTAL_STRIPS:
				HStrip(geomPos, stripGeomWidth, texPos, stripTexWigth);
				break;
			}
			geomPos += stripGeomWidth;
			texPos += stripTexWigth;
		};
	};

	if (quadType == QUAD_NET_FROM_LLC || quadType == QUAD_NET_FROM_URC)
	{
		int quadIdx{ 0 };
		auto dims = GetMultipliers(Texture, moduleCount);
		
		const float delthaQuadWidth{ 2.0f / ((float)dims.Width) };
		const float delthaQuadHeight{ 2.0f / ((float)dims.Height) };

		const float delthaTextWidth{ 1.0f / ((float)dims.Width) };
		const float delthaTextHeight{ 1.0f / ((float)dims.Height) };

		float quadLeftLowerCorner_x{ -1.0f };
		float quadLeftLowerCorner_y{ (quadType == Pattern::QUAD_NET_FROM_LLC) ? -1.0f : 1.0f - delthaQuadHeight };

		float texLeftLowerCorner_x{ 0.0f };
		float texLeftLowerCorner_y{ (quadType == Pattern::QUAD_NET_FROM_LLC) ? 0.0f : 1.0f - delthaTextHeight };


		for (int i{ 0 }; i < dims.Height; i++)
		{
			quadLeftLowerCorner_x = (quadType == QUAD_NET_FROM_LLC) ? -1.0f : 1.0f - delthaQuadWidth;
			texLeftLowerCorner_x = (quadType == QUAD_NET_FROM_LLC) ? 0.0f : 1.0f - delthaTextWidth;
			for (int j{ 0 }; j < dims.Width; j++)
			{
				int fbIndex = (quadIdx == 0) ? GetOldest() : (quadIdx == (moduleCount - 1)) ? GetNewest() : (int)(GetOldest() + ((float)quadIdx) * fStep) % bufferCount;
				glBindTexture(GL_TEXTURE_2D, fbos[fbIndex].textureId);

				glBegin(GL_QUADS);

				//lower left
				glTexCoord2d(texLeftLowerCorner_x, texLeftLowerCorner_y);
				glVertex2f(quadLeftLowerCorner_x, quadLeftLowerCorner_y);

				//upper left	
				glTexCoord2d(texLeftLowerCorner_x, texLeftLowerCorner_y + delthaTextHeight);
				glVertex2f(quadLeftLowerCorner_x, quadLeftLowerCorner_y + delthaQuadHeight);

				//upper right	
				glTexCoord2d(texLeftLowerCorner_x + delthaTextWidth, texLeftLowerCorner_y + delthaTextHeight);
				glVertex2f(quadLeftLowerCorner_x + delthaQuadWidth, quadLeftLowerCorner_y + delthaQuadHeight);

				//lower right
				glTexCoord2d(texLeftLowerCorner_x + delthaTextWidth, texLeftLowerCorner_y);
				glVertex2f(quadLeftLowerCorner_x + delthaQuadWidth, quadLeftLowerCorner_y);

				glEnd();

				if (quadType == QUAD_NET_FROM_LLC)
				{
					quadLeftLowerCorner_x += delthaQuadWidth;
					texLeftLowerCorner_x += delthaTextWidth;
				}
				else
				{
					quadLeftLowerCorner_x -= delthaQuadWidth;
					texLeftLowerCorner_x -= delthaTextWidth;
				}

				quadIdx++;
			}

			if (quadType == QUAD_NET_FROM_LLC)
			{
				quadLeftLowerCorner_y += delthaQuadHeight;
				texLeftLowerCorner_y += delthaTextHeight;
			}
			else
			{
				quadLeftLowerCorner_y -= delthaQuadHeight;
				texLeftLowerCorner_y -= delthaTextHeight;
			}
		}
	}

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

static FFGLTextureStruct GetMultipliers(FFGLTextureStruct textureDesc, int modulesCount)
{
	const float ratio { (float)textureDesc.HardwareHeight / (float)textureDesc.HardwareWidth };

	DWORD n{ (DWORD)sqrt(modulesCount) };
	DWORD m{ (DWORD)(ratio * (float)n) };

	DWORD foundN{ 0 };
	DWORD foundM{ 0 };

	while (n*m <= modulesCount)
	{
		foundN = n;
		foundM = m;
		m = (DWORD)(ratio * (float)(++n));
	}

	auto nextN{ foundN + 1 };
	auto nextM{ foundM + 1 };
	
	if (nextN * nextM <= modulesCount)
	{
		return FFGLTextureStruct{ nextN, nextM };
	}
	else
	{
		auto m1{ nextN * foundM };
		auto m2{ foundN * nextM };

		if (m1 < m2 && m2 <= modulesCount)
			return FFGLTextureStruct{ foundN, nextM };

		if (m2 < m1 && m1 <= modulesCount)
			return FFGLTextureStruct{ nextN, foundM };
	}
	
	return FFGLTextureStruct{ foundN, foundM };
}
