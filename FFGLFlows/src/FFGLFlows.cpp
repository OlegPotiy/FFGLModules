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




////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FFGLFlows::FFGLFlows() :CFreeFrameGLPlugin()
{
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

	parameterDefinitions = new std::map<int, ParamDefinition>{
		{ (int)ParamNames::NOISE_FACTOR, ParamDefinition{ "Noise factor", FF_TYPE_STANDARD, &this->alphaNoisesTexture } },
		{ (int)ParamNames::IMG_BLENDING_FACTOR, ParamDefinition{ "Img fraction", FF_TYPE_STANDARD, &this->alphaImageTexture } },
		{ (int)ParamNames::NOISES_TEXTURES_COUNT, ParamDefinition{ "Noises count", FF_TYPE_STANDARD, &this->noiseTexturesCountFactor } },
		{ (int)ParamNames::NOISES_SCALE, ParamDefinition{ "Noises scale", FF_TYPE_STANDARD, &this->noiseDimScale } },

		{ (int)ParamNames::VELOCITY, ParamDefinition{ "Velosity", FF_TYPE_STANDARD, &this->velocity } },
		{ (int)ParamNames::VELOCITY_SCALE, ParamDefinition{ "Velosity Scale", FF_TYPE_STANDARD, &this->velocityScale } },

		{ (int)ParamNames::XFACTOR, ParamDefinition{ "X Factor", FF_TYPE_STANDARD, &this->xFactor } },
		{ (int)ParamNames::YFACTOR, ParamDefinition{ "Y Factor", FF_TYPE_STANDARD, &this->yFactor } },

		{ (int)ParamNames::XSHIFT, ParamDefinition{ "X Shift", FF_TYPE_STANDARD, &this->xShift } },
		{ (int)ParamNames::YSHIFT, ParamDefinition{ "Y Shift", FF_TYPE_STANDARD, &this->yShift } }
	};

	for (auto param : *parameterDefinitions)
	{
		auto value = param.second;
		SetParamInfo(param.first, value.paramName.c_str(), value.paramType, value.valueStorage);
	}
}

FFGLFlows::~FFGLFlows()
{
	parameterDefinitions->clear();
	delete parameterDefinitions;
}




DWORD FFGLFlows::InitGL(const FFGLViewportStruct *vp)
{
	this->vertexShaderCode = STRINGIFY(
		void main()
	{
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_TexCoord[1] = gl_MultiTexCoord1;
		gl_TexCoord[2] = gl_MultiTexCoord2;
		gl_FrontColor = gl_Color;
	});


	this->fragmentShaderCode = STRINGIFY(
		uniform sampler2D texture0;
	uniform sampler2D texture1;
	uniform sampler2D texture2;
	uniform float alpha;
	uniform float imgFactor;
	uniform float velocity;
	uniform float velocityScale;
	uniform float dx;
	uniform float dy;
	uniform int fieldSource;

	float getWeight(vec4 cVal)
	{
		return cVal.r + cVal.g + cVal.b + cVal.a;
	}

	vec2  getDirectField()
	{
		vec2 field = vec2(0.0);
		vec4 texValue = texture2D(texture2, gl_TexCoord[2]);
		field = texValue.xy - vec2(0.5);
		return field * texValue.z;
	}

	vec2 getSobel()
	{
		bool isInsideOfArea = false;
		vec4 gradient = vec4(0.0);
		vec2 texCoords = gl_TexCoord[2].st;

		vec4 z1 = getWeight(texture2D(texture2, vec2(texCoords.x - dx, texCoords.y + dy)));
		vec4 z2 = getWeight(texture2D(texture2, vec2(texCoords.x, texCoords.y + dy)));
		vec4 z3 = getWeight(texture2D(texture2, vec2(texCoords.x + dx, texCoords.y + dy)));
		vec4 z4 = getWeight(texture2D(texture2, vec2(texCoords.x - dx, texCoords.y)));

		vec4 z6 = getWeight(texture2D(texture2, vec2(texCoords.x + dx, texCoords.y)));
		vec4 z7 = getWeight(texture2D(texture2, vec2(texCoords.x - dx, texCoords.y - dy)));
		vec4 z8 = getWeight(texture2D(texture2, vec2(texCoords.x, texCoords.y - dy)));
		vec4 z9 = getWeight(texture2D(texture2, vec2(texCoords.x + dx, texCoords.y - dy)));

		gradient.y = (z7 + 2 * z8 + z9) - (z1 + 2 * z2 + z3);
		gradient.x = -(z3 + 2 * z6 + z9) + (z1 + 2 * z4 + z7);

		isInsideOfArea = (z1 == z2) && (z2 == z3) && (z3 == z4) && (z4 == z6) && (z6 == z7) && (z7 == z8) && (z8 == z9);
		if (!isInsideOfArea) {
			return gradient.xy;
		}
		else {
			return vec2(0.5) - texCoords;
		}
	}

	void main()
	{
		vec2 texCoord = gl_TexCoord[0].st;
		vec2 coords = vec2(-1.0, -1.0) + 2.0 * texCoord;
		vec4 imgColor = texture2D(texture2, gl_TexCoord[2]);


		//"	vec2 field = velocity * texture2D(texture2, gl_TexCoord[2]);"

		//"	vec2 field = velocity * getSobel(gl_TexCoord[2]);"

		vec2 field = velocityScale * (velocity - 0.5) * getDirectField();

		vec2 samplerCoord = texCoord - field;
		vec4 srcColor = texture2D(texture0, samplerCoord);
		vec4 noiseColor = texture2D(texture1, vec2(texCoord.s, texCoord.t));
		gl_FragColor = (1 - alpha) * (((1 - imgFactor) * srcColor) + (imgFactor * imgColor)) + alpha * noiseColor;

	});


	m_extensions.Initialize();
	m_shader.SetExtensions(&m_extensions);

	maxHorisontalNoiseDim = vp->width;
	maxVerticalNoiseDim = vp->height;

	bool lIsExtSupported = m_extensions.isExtensionSupported("GL_ARB_vertex_buffer_object") &&
		m_extensions.isExtensionSupported("GL_ARB_texture_float") &&
		m_extensions.isExtensionSupported("GL_ARB_shader_objects");

	if (!lIsExtSupported)
		return FF_FAIL;

	const GLubyte * glslInfo = glGetString(GL_SHADING_LANGUAGE_VERSION);

	int isCompiled = m_shader.Compile(this->vertexShaderCode->c_str(), this->fragmentShaderCode->c_str());

	if (isCompiled != 1)
		return FF_FAIL;

	this->fbos = CreateFBO(vp);
	if (this->fbos == nullptr)
		return FF_FAIL;

	m_shader.BindShader();

	GLint textureUnitParam = m_shader.FindUniform("texture0");
	m_extensions.glUniform1iARB(textureUnitParam, 0);

	textureUnitParam = m_shader.FindUniform("texture1");
	m_extensions.glUniform1iARB(textureUnitParam, 1);

	textureUnitParam = m_shader.FindUniform("texture2");
	m_extensions.glUniform1iARB(textureUnitParam, 2);

	m_shader.UnbindShader();


	delete this->vertexShaderCode;
	delete this->fragmentShaderCode;

	return FF_SUCCESS;
}

DWORD FFGLFlows::DeInitGL()
{

	DeleteNoiseTextures();



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

	if (this->noiseTexturesIds == NULL)
		this->CreateTextures(this->noiseDimScale * this->maxHorisontalNoiseDim,
			this->noiseDimScale * this->maxVerticalNoiseDim,
			this->ntexCount);


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


	// set rendering destination to FBO
	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, dstRenderBuffer);
	glClear(GL_COLOR_BUFFER_BIT);
	m_shader.BindShader();


	GLuint paramId = m_shader.FindUniform("dx");
	m_extensions.glUniform1fARB(paramId, 1.0f / (float)Texture.Width);

	paramId = m_shader.FindUniform("dy");
	m_extensions.glUniform1fARB(paramId, 1.0f / (float)Texture.Height);

	paramId = m_shader.FindUniform("alpha");
	m_extensions.glUniform1fARB(paramId, alphaNoisesTexture);

	paramId = m_shader.FindUniform("imgFactor");
	m_extensions.glUniform1fARB(paramId, alphaImageTexture);

	paramId = m_shader.FindUniform("velocity");
	m_extensions.glUniform1fARB(paramId, this->velocity);

	paramId = m_shader.FindUniform("velocityScale");
	m_extensions.glUniform1fARB(paramId, this->velocityScale);

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
	m_shader.UnbindShader();

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
	this->iCounter %= this->ntexCount;

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
	if (this->noiseTexturesIds != NULL)
	{
		glDeleteTextures(this->ntexCount, this->noiseTexturesIds);
		delete this->noiseTexturesIds;
		this->noiseTexturesIds = NULL;
	}
}

void FFGLFlows::CreateTextures(int width, int height, int texNum)
{
	int lut[256];
	int *phase = new int[width * height];
	GLubyte *pat = new GLubyte[width * height];
	this->noiseTexturesIds = new GLuint[texNum];


	int i, k, t;

	for (i = 0; i < 256; i++) lut[i] = i < 127 ? 0 : 255;
	for (i = 0; i < height * width; i++)
		phase[i] = rand() % 256;

	glGenTextures(texNum, this->noiseTexturesIds);

	for (k = 0; k < texNum; k++)
	{
		t = k * 256 / texNum;
		for (i = 0; i < width*height; i++)
			pat[i] = lut[(t + phase[i]) % 255];

		glBindTexture(GL_TEXTURE_2D, this->noiseTexturesIds[k]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pat);
	}

	delete phase;
	delete pat;

}


DWORD FFGLFlows::GetParameter(DWORD dwIndex)
{
	DWORD dwRet;

	auto result = parameterDefinitions->find(dwIndex);

	if (result != parameterDefinitions->end())
	{
		*((float *)(unsigned)(&dwRet)) = *(result->second.valueStorage);
		return dwRet;
	}

	return FF_FAIL;
	
}

DWORD FFGLFlows::SetParameter(const SetParameterStruct* pParam)
{	
	if (pParam != NULL)
	{

		float newValue = *((float *)(unsigned)&(pParam->NewParameterValue));

		auto result = parameterDefinitions->find(pParam->ParameterNumber);

		if (result != parameterDefinitions->end())
		{
			if (newValue != *(result->second.valueStorage))
			{
				*(result->second.valueStorage) = newValue;

				if (pParam->ParameterNumber == (int)ParamNames::NOISES_TEXTURES_COUNT ||
					pParam->ParameterNumber == (int)ParamNames::NOISES_SCALE)
				{
					this->DeleteNoiseTextures();
					auto newNtexCount = mulFtoI(noiseTexturesCountFactor, maxNoiseTexturesAmount);
					this->ntexCount = newNtexCount == 0 ? 1 : newNtexCount;
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
