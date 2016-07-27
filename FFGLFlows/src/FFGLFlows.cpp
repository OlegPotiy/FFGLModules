#include "FFGLFlows.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo(
	FFGLFlows::CreateInstance,	// Create method
	"FLOW",						// Plugin unique ID
	"FFGLFlows",				// Plugin name											
	1,						   	// API major version number 											
	000,						// API minor version number	
	1,							// Plugin major version number
	000,						// Plugin minor version number
	FF_EFFECT,					// Plugin type
	"FFGL Flows Visualisation",	// Plugin description
	"by Oleg Potiy"				// About
	);


std::string ShaderCodeConcat(std::string* fscUniform, std::string* fscFuncCode, std::string* fieldCode, std::string* fscAdvectionFunc)
{
	std::string analyticShaderCode{ *fscUniform + *fscFuncCode + ((fieldCode == nullptr) ? "" : *fieldCode) + "; return field;}" + *fscAdvectionFunc };
	return analyticShaderCode;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FFGLFlows::FFGLFlows() :CFreeFrameGLPlugin()
{
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

	parameterDefinitions = new std::map<int, ParamDefinition>
	{
		{ (int)ParamNames::FUNC_DEF, ParamDefinition{ "Field def", FF_TYPE_TEXT, nullptr} },
		{ (int)ParamNames::NOISE_FACTOR, ParamDefinition{ "Noise factor", FF_TYPE_STANDARD, &this->alphaNoisesTexture } },
		{ (int)ParamNames::IMG_BLENDING_FACTOR, ParamDefinition{ "Img fraction", FF_TYPE_STANDARD, &this->alphaImageTexture } },
		{ (int)ParamNames::NOISES_TEXTURES_COUNT, ParamDefinition{ "Noises count", FF_TYPE_STANDARD, &this->noiseTexturesCountFactor } },
		{ (int)ParamNames::NOISES_SCALE, ParamDefinition{ "Noises scale", FF_TYPE_STANDARD, &this->noiseDimScale } },
		{ (int)ParamNames::OPERATOR_TYPE, ParamDefinition{ "Operator type", FF_TYPE_STANDARD, &this->operatorTypeFactor } },	

		{ (int)ParamNames::XFACTOR, ParamDefinition{ "X Factor", FF_TYPE_STANDARD, &this->xFactor } },
		{ (int)ParamNames::YFACTOR, ParamDefinition{ "Y Factor", FF_TYPE_STANDARD, &this->yFactor } },

		{ (int)ParamNames::XSHIFT, ParamDefinition{ "X Shift", FF_TYPE_STANDARD, &this->xShift } },
		{ (int)ParamNames::YSHIFT, ParamDefinition{ "Y Shift", FF_TYPE_STANDARD, &this->yShift } },

		{ (int)ParamNames::VELOCITY, ParamDefinition{ "Velosity", FF_TYPE_STANDARD, &this->velocity } },
		{ (int)ParamNames::VELOCITY_SCALE, ParamDefinition{ "Velosity Scale", FF_TYPE_STANDARD, &this->velocityScale } }

		
	};

	char* emptyStr = "";
	//SetParamInfo((int)ParamNames::FUNC_DEF, "Function definition", FF_TYPE_STANDARD, &this->opertorCode);

	for (auto param : *parameterDefinitions)
	{
		auto value = param.second;
		if (value.paramType == FF_TYPE_STANDARD)
		{
			SetParamInfo(param.first, value.paramName.c_str(), value.paramType, value.floatValueStorage);
		} 
		else if (value.paramType == FF_TYPE_TEXT)
		{
			SetParamInfo(param.first, value.paramName.c_str() , FF_TYPE_TEXT, this->fieldCode.c_str());
		}

	}
}

FFGLFlows::~FFGLFlows()
{
	parameterDefinitions->clear();	
	delete parameterDefinitions;
}




DWORD FFGLFlows::InitGL(const FFGLViewportStruct *vp)
{
	using namespace std;

	this->vertexShaderCode = STRINGIFY(
		void main()
	{
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_TexCoord[1] = gl_MultiTexCoord1;
		gl_TexCoord[2] = gl_MultiTexCoord2;
		gl_FrontColor = gl_Color;
	});

	// Pixel shader: uniform variables
	this->fscUniform = STRINGIFY(	
	
	uniform sampler2D texture0;
	uniform sampler2D texture1;
	uniform sampler2D texture2;

	uniform float dx;
	uniform float dy;

	uniform float alpha;
	uniform float imgFactor;
	uniform float velocity;
	uniform float velocityScale;

	uniform float noiseScale;	

	);

	// Analytic field operator
	this->fscAlnalyticStart = STRINGIFY(

	vec2 getField()
	{
		vec2 coords = 2*(gl_TexCoord[2].st - 0.5);
		float x = coords.x;
		float y = coords.y;
		vec2 field = vec2(0.0);		
		field = 
	);	


	// 3x3 sobel operator
	this->fscSobel = STRINGIFY(

	float getWeight(vec4 cVal) 
	{ 
		return cVal.r + cVal.g + cVal.b + cVal.a; 
	}

	vec2 getField()
	{		
		vec2 gradient = vec2(0.0);
		vec2 texCoords = gl_TexCoord[2].xy;
		
		float z1 = getWeight(texture2D(texture2, vec2(texCoords.x - dx, texCoords.y + dy)));
		float z2 = getWeight(texture2D(texture2, vec2(texCoords.x, texCoords.y + dy)));
		float z3 = getWeight(texture2D(texture2, vec2(texCoords.x + dx, texCoords.y + dy)));
		float z4 = getWeight(texture2D(texture2, vec2(texCoords.x - dx, texCoords.y)));

		float z6 = getWeight(texture2D(texture2, vec2(texCoords.x + dx, texCoords.y)));
		float z7 = getWeight(texture2D(texture2, vec2(texCoords.x - dx, texCoords.y - dy)));
		float z8 = getWeight(texture2D(texture2, vec2(texCoords.x, texCoords.y - dy)));
		float z9 = getWeight(texture2D(texture2, vec2(texCoords.x + dx, texCoords.y - dy)));

		gradient.y = (z7 + 2 * z8 + z9) - (z1 + 2 * z2 + z3);
		gradient.x = -(z3 + 2 * z6 + z9) + (z1 + 2 * z4 + z7);

		return gradient;
	}

	);

	// Get field direction from the image itself (through the colour-to-value mapping)
	this->fscDirect = STRINGIFY(

	vec2  getField()
	{
		vec2 field = vec2(0.0);
		vec2 texCoords = gl_TexCoord[2].xy;

		vec4 texValue = texture2D(texture2, texCoords);
		field = texValue.xy - vec2(0.5);
		return field * texValue.z;
	}

	);

	// Advection operator
	this->fscAdvectionFunc = STRINGIFY(
	void main()
	{
		vec2 texCoord = gl_TexCoord[0].st;
		vec2 coords = vec2(-1.0, -1.0) + 2.0 * texCoord;
		vec4 imgColor = texture2D(texture2, gl_TexCoord[2]);

		vec2 field = velocityScale * (velocity - 0.5) * getField();

		vec2 samplerCoord = texCoord - field;
		vec4 srcColor = texture2D(texture0, samplerCoord);
		vec4 noiseColor = texture2D(texture1, 0.5*vec2(1-noiseScale) + noiseScale * texCoord);
		gl_FragColor = (1 - alpha) * (((1 - imgFactor) * srcColor) + (imgFactor * imgColor)) + alpha * noiseColor;
	}
	);



	m_extensions.Initialize();

	analyticFieldShader.SetExtensions(&m_extensions);
	sobelFieldShader.SetExtensions(&m_extensions);
	directFieldShader.SetExtensions(&m_extensions);

	
	bool lIsExtSupported = m_extensions.isExtensionSupported("GL_ARB_vertex_buffer_object") &&
		m_extensions.isExtensionSupported("GL_ARB_texture_float") &&
		m_extensions.isExtensionSupported("GL_ARB_shader_objects");

	if (!lIsExtSupported)
		return FF_FAIL;

	string sobelShaderCode{ *(this->fscUniform) + *(this->fscSobel) + *(this->fscAdvectionFunc) };
	sobelFieldShader.Compile(this->vertexShaderCode->c_str(), sobelShaderCode.c_str());

	string directShaderCode{ *(this->fscUniform) + *(this->fscDirect) + *(this->fscAdvectionFunc) };
	directFieldShader.Compile(this->vertexShaderCode->c_str(), directShaderCode.c_str());

	//string analyticShaderCode{ *(this->fscUniform) + *(this->fscAlnalyticStart) + this->fieldCode + "; return field;}" + *(this->fscAdvectionFunc) };
	auto analyticShaderCode = ShaderCodeConcat(this->fscUniform, this->fscAlnalyticStart, &this->fieldCode, this->fscAdvectionFunc);
	analyticFieldShader.Compile(this->vertexShaderCode->c_str(), analyticShaderCode.c_str());

	//delete this->vertexShaderCode;
	//delete this->fscUniform;
	//delete this->fscAlnalyticStart;
	//delete this->fscDirect;
	//delete this->fscSobel;
	//delete this->fscAdvectionFunc;

	this->fbos = CreateFBO(vp);
	if (this->fbos == nullptr)
		return FF_FAIL;

	maxHorisontalNoiseDim = vp->width;
	maxVerticalNoiseDim = vp->height;

	

	return FF_SUCCESS;
}



DWORD FFGLFlows::DeInitGL()
{

	DeleteNoiseTextures();

	if (this->patterns != nullptr)
		delete this->patterns;

	return FF_SUCCESS;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD FFGLFlows::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{

	if (pGL->numInputTextures < 1)
		return FF_FAIL;

	if (pGL->inputTextures[0] == NULL)
		return FF_FAIL;

	FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

	//get the max s,t that correspond to the 
	//width,height of the used portion of the allocated texture space
	FFGLTexCoords maxCoords = GetMaxGLTexCoords(Texture);

	GLint systemCurrentFbo;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &systemCurrentFbo);


	GLuint srcTexture;
	GLuint dstRenderBuffer;
	GLuint dstTexture;

		

	if (fbos->isEven)
	{
		// Even:
		srcTexture = fbos->oddTextureId;
		dstRenderBuffer = fbos->evenBufferId;
		dstTexture = fbos->evenTextureId;

		fbos->isEven = false;
	}
	else
	{
		// Odd:
		srcTexture = IsFirstFrame ? Texture.Handle : fbos->evenTextureId;
		dstRenderBuffer = fbos->oddBufferId;
		dstTexture = fbos->oddTextureId;

		fbos->isEven = true;
	}

	IsFirstFrame = false;




	glEnable(GL_TEXTURE_2D);


	CreateTextures();

	// set rendering destination to FBO
	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, dstRenderBuffer);
	glClear(GL_COLOR_BUFFER_BIT);


	FFGLShader* usedShader = this->operatorTypeFactor > 0.5 ? (&directFieldShader) : (&sobelFieldShader);

	if (this->operatorTypeFactor == 0)
	{
		usedShader = &analyticFieldShader;
	}

	usedShader->BindShader();
	
	m_extensions.glUniform1iARB(usedShader->FindUniform("texture0"), 0);
	m_extensions.glUniform1iARB(usedShader->FindUniform("texture1"), 1);
	m_extensions.glUniform1iARB(usedShader->FindUniform("texture2"), 2);

	m_extensions.glUniform1fARB(usedShader->FindUniform("alpha"), 0.1*alphaNoisesTexture);
	m_extensions.glUniform1fARB(usedShader->FindUniform("imgFactor"), alphaImageTexture);
	m_extensions.glUniform1fARB(usedShader->FindUniform("velocity"), this->velocity);
	m_extensions.glUniform1fARB(usedShader->FindUniform("velocityScale"), this->velocityScale);
	m_extensions.glUniform1fARB(usedShader->FindUniform("noiseScale"), this->noiseDimScale);
	

	m_extensions.glUniform1fARB(usedShader->FindUniform("dx"), 1.0f / (float)Texture.Width);
	m_extensions.glUniform1fARB(usedShader->FindUniform("dy"), 1.0f / (float)Texture.Height);

	m_extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, srcTexture);

	m_extensions.glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, this->noiseTexturesIds[iCounter]);

	m_extensions.glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, Texture.Handle);

	glBegin(GL_QUADS);

	//lower left
	m_extensions.glMultiTexCoord2d(GL_TEXTURE0, 0, 0);
	m_extensions.glMultiTexCoord2d(GL_TEXTURE1, 0, 0);
	m_extensions.glMultiTexCoord2d(GL_TEXTURE2, 0, 0);
	glVertex2f(-1, -1);

	//upper left
	m_extensions.glMultiTexCoord2d(GL_TEXTURE0, 0, maxCoords.t);
	m_extensions.glMultiTexCoord2d(GL_TEXTURE1, 0, maxCoords.t);
	m_extensions.glMultiTexCoord2d(GL_TEXTURE2, 0, maxCoords.t);
	glVertex2f(-1, 1);

	//upper right
	m_extensions.glMultiTexCoord2d(GL_TEXTURE0, maxCoords.s, maxCoords.t);
	m_extensions.glMultiTexCoord2d(GL_TEXTURE1, maxCoords.s, maxCoords.t);
	m_extensions.glMultiTexCoord2d(GL_TEXTURE2, maxCoords.s, maxCoords.t);
	glVertex2f(1, 1);

	//lower right
	m_extensions.glMultiTexCoord2d(GL_TEXTURE0, maxCoords.s, 0);
	m_extensions.glMultiTexCoord2d(GL_TEXTURE1, maxCoords.s, 0);
	m_extensions.glMultiTexCoord2d(GL_TEXTURE2, maxCoords.s, 0);
	glVertex2f(1, -1);
	glEnd();

	//unbind the input texture
	glBindTexture(GL_TEXTURE_2D, 0);
	usedShader->UnbindShader();

	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, systemCurrentFbo);

	glBindTexture(GL_TEXTURE_2D, dstTexture);

	glBegin(GL_QUADS);

	//lower left	
	glTexCoord2d(0, 0);
	glVertex2f(-1, -1);

	//upper left
	glTexCoord2d(0, maxCoords.t);
	glVertex2f(-1, 1);

	//upper right
	glTexCoord2d(maxCoords.s, maxCoords.t);
	glVertex2f(1, 1);

	//lower right
	glTexCoord2d(maxCoords.s, 0);
	glVertex2f(1, -1);
	glEnd();


	glMatrixMode(GL_PROJECTION);

	//Restoring projection matrix
	glPopMatrix();

	//unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	//disable texturemapping
	glDisable(GL_TEXTURE_2D);

	//restore default color
	glColor4f(1.f, 1.f, 1.f, 1.f);


	this->iCounter++;
	this->iCounter %= this->maxNoiseTexturesAmount;

	return FF_SUCCESS;
}


FFGLFlows::FBOPair* FFGLFlows::CreateFBO(const FFGLViewportStruct *viewportStruct)
{

	GLenum status;
	FBOPair* pair = new FBOPair();

	// Init evenBufferId/evenTextureId
	m_extensions.glGenFramebuffersEXT(1, &pair->evenBufferId);
	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pair->evenBufferId);

	glGenTextures(1, &pair->evenTextureId);
	glBindTexture(GL_TEXTURE_2D, pair->evenTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, viewportStruct->width, viewportStruct->height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	m_extensions.glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, pair->evenTextureId, 0);

	status = m_extensions.glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
		return nullptr;

	// Init oddBufferId/oddTextureId
	m_extensions.glGenFramebuffersEXT(1, &pair->oddBufferId);
	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pair->oddBufferId);

	glGenTextures(1, &pair->oddTextureId);
	glBindTexture(GL_TEXTURE_2D, pair->oddTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, viewportStruct->width, viewportStruct->height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	m_extensions.glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, pair->oddTextureId, 0);

	status = m_extensions.glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
		return nullptr;

	pair->isEven = false;

	return pair;

}


void FFGLFlows::DeleteNoiseTextures()
{
	
	if (this->noiseTexturesIds != nullptr)
	{
		glDeleteTextures(this->ntexCount, this->noiseTexturesIds);
		delete this->noiseTexturesIds;
		this->noiseTexturesIds = nullptr;
	}
}

void FFGLFlows::CreateNoises()
{	
	const int patternSize{ (int)(this->maxVerticalNoiseDim * this->maxHorisontalNoiseDim) };
	
	this->patterns = new GLubyte[this->maxNoiseTexturesAmount * patternSize];

	GLubyte lut[256];
	for (int i = 0; i < 256; i++) lut[i] = i < 127 ? 0 : 255;
	
	GLubyte* phase = new GLubyte[patternSize];
	
	for (int i = 0; i < patternSize; i++)
		phase[i] = rand() % 256;


	for (int i = 0; i < this->maxNoiseTexturesAmount; i++)
	{
		int arg{ i * 256 / this->maxNoiseTexturesAmount };
		int offset{ i * patternSize };

		for (int j = 0; j < patternSize; j++)
		{
			patterns[j + offset] = lut[(arg + phase[j]) % 255];
		};
	}

	delete phase;	
}

void FFGLFlows::CreateTextures()
{
	const int patternSize{ (int)(this->maxVerticalNoiseDim * this->maxHorisontalNoiseDim) };

	if (this->patterns == nullptr)
	{
		CreateNoises();
	}

	if (this->noiseTexturesIds == nullptr)
	{
		this->noiseTexturesIds = new GLuint[maxNoiseTexturesAmount];
		
		glGenTextures(maxNoiseTexturesAmount, this->noiseTexturesIds);

		for (int i = 0; i < maxNoiseTexturesAmount; i++)
		{
			int offset{ i * patternSize };

			glBindTexture(GL_TEXTURE_2D, this->noiseTexturesIds[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, this->maxHorisontalNoiseDim, this->maxVerticalNoiseDim, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &(this->patterns[offset]));
		}
	}
	
}


DWORD FFGLFlows::GetParameter(DWORD dwIndex)
{
	DWORD dwRet;

	auto result = parameterDefinitions->find(dwIndex);

	if (result != parameterDefinitions->end())
	{	
		if (result->first == (int)ParamNames::FUNC_DEF)
		{
		}
		else
		{
			*((float *)(unsigned)(&dwRet)) = *(result->second.floatValueStorage);
			return dwRet;
		}
	}

	return FF_FAIL;
	
}

DWORD FFGLFlows::SetParameter(const SetParameterStruct* pParam)
{	
	if (pParam != NULL)
	{
		auto result = parameterDefinitions->find(pParam->ParameterNumber);

		if (result != parameterDefinitions->end())
		{
		
			if (result->first == (int)ParamNames::FUNC_DEF)
			{		
				if (strlen((char*)pParam->NewParameterValue) > 0)
				{
					if (this->fieldCode.compare((char*)pParam->NewParameterValue) != 0)
					{
						this->fieldCode.assign((char*)pParam->NewParameterValue);

						auto analyticShaderCode = ShaderCodeConcat(this->fscUniform, this->fscAlnalyticStart, &this->fieldCode, this->fscAdvectionFunc);
						analyticFieldShader.Compile(this->vertexShaderCode->c_str(), analyticShaderCode.c_str());
					}
				}
			}
			else
			{
				float newValue = *((float *)(unsigned)&(pParam->NewParameterValue));

				if (newValue != *(result->second.floatValueStorage))
				{
					*(result->second.floatValueStorage) = newValue;
				}
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
