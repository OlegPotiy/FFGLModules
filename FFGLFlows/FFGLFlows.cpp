#include "../FFGL Lib/FFGL.h"
#include "../FFGL Lib/FFGLLib.h"
#include "FFGLFlows.h"
#include <gl\GLU.h>
#include <math.h>
#include <string>
#define USE_VBO

// Parameters
#define FFPARAM_VALUE1_NOISE_BLENDING_FACTOR	(0)
#define FFPARAM_VALUE2_TEXTURE_BLENDING_FACTOR	(1)
#define FFPARAM_VALUE3_NOISE_TEXTURES_AMOUNT	(2)
#define FFPARAM_VALUE4_NOISE_TEXTURES_DIMENSION	(3)
#define FFPARAM_VALUE5_XFACTOR	(4)
#define FFPARAM_VALUE6_YFACTOR	(5)
#define FFPARAM_VALUE_VELOCITY	(6)
#define FFPARAM_VALUE_VELOCITY_SCALE (7)
#define FFPARAM_VALUE_XSHIFT	(8)
#define FFPARAM_VALUE_YSHIFT	(9)


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

static const int MaxPlanes = 200;


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FFGLFlows::FFGLFlows() :CFreeFrameGLPlugin()
{
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

	SetParamInfo(FFPARAM_VALUE1_NOISE_BLENDING_FACTOR, "Noise fraction", FF_TYPE_STANDARD, this->alphaNoisesTexture);
	SetParamInfo(FFPARAM_VALUE2_TEXTURE_BLENDING_FACTOR, "Img fraction", FF_TYPE_STANDARD, this->alphaImageTexture);
	SetParamInfo(FFPARAM_VALUE3_NOISE_TEXTURES_AMOUNT, "Noises amount", FF_TYPE_STANDARD, ((float)this->noiseTexturesAmount) / ((float)this->maxNoiseTexturesAmount));
	SetParamInfo(FFPARAM_VALUE4_NOISE_TEXTURES_DIMENSION, "Noises dim", FF_TYPE_STANDARD, ((float)this->noiseTexturesDimension) / ((float)this->maxNoiseTexturesDimension));

	SetParamInfo(FFPARAM_VALUE5_XFACTOR, "X Factor", FF_TYPE_STANDARD, this->xFactor);
	SetParamInfo(FFPARAM_VALUE6_YFACTOR, "Y Factor", FF_TYPE_STANDARD, this->yFactor);
	
	SetParamInfo(FFPARAM_VALUE_VELOCITY, "Velosity", FF_TYPE_STANDARD, this->velocity);
	SetParamInfo(FFPARAM_VALUE_VELOCITY_SCALE, "Velosity Scale", FF_TYPE_STANDARD, this->velocityScale);
	
	SetParamInfo(FFPARAM_VALUE_XSHIFT, "X Shift", FF_TYPE_STANDARD, this->xShift);
	SetParamInfo(FFPARAM_VALUE_YSHIFT, "Y Shift", FF_TYPE_STANDARD, this->yShift);
}

FFGLFlows::~FFGLFlows()
{
}

static const std::string vertexShaderCode = STRINGIFY(
void main()
{
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
  gl_TexCoord[0] = gl_MultiTexCoord0;
  gl_TexCoord[1] = gl_MultiTexCoord1;
  gl_TexCoord[2] = gl_MultiTexCoord2;
  gl_FrontColor = gl_Color;
});


static const std::string fragmentShaderCode = STRINGIFY(
uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float alpha;
uniform float velocity;
uniform float velocityScale;
uniform float dx;
uniform float dy;


float getWeight(vec4 cVal)
{
	return cVal.r + cVal.g + cVal.b + cVal.a;
}

vec2  getDirectField()
{
	vec2 field = vec2(0.0);
	vec4 texValue = texture2D(texture2, gl_TexCoord[2]);
	field =  texValue.xy - vec2(0.5);
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

	gradient.y = (z7 + 2*z8 + z9) - (z1 + 2*z2 + z3);
	gradient.x = -(z3 + 2*z6 + z9) + (z1 + 2*z4 + z7);

	isInsideOfArea = (z1 == z2) && (z2 == z3) && (z3 == z4) && (z4 == z6) && (z6 == z7) && (z7 == z8) && (z8 == z9);
	if (!isInsideOfArea) {
		return gradient.xy;
	} else {
		return vec2(0.5) - texCoords;
	}
}

void main()
{
   vec2 texCoord = gl_TexCoord[0].st;
	vec2 coords = vec2(-1.0,-1.0) + 2.0 * texCoord;


//"	vec2 field = velocity * texture2D(texture2, gl_TexCoord[2]);"

//"	vec2 field = velocity * getSobel(gl_TexCoord[2]);"

	vec2 field = velocityScale * (velocity - 0.5) * getDirectField();

	vec2 samplerCoord = texCoord - field;
	vec4 srcColor = texture2D(texture0, samplerCoord);
	vec4 noiseColor = texture2D(texture1, vec2(texCoord.s, texCoord.t));
	gl_FragColor = (1-alpha) * srcColor + alpha * noiseColor;
});



DWORD FFGLFlows::InitGL(const FFGLViewportStruct *vp)
{
	m_extensions.Initialize();
	m_shader.SetExtensions(&m_extensions);

	bool lIsExtSupported = m_extensions.isExtensionSupported("GL_ARB_vertex_buffer_object") &&
		m_extensions.isExtensionSupported("GL_ARB_texture_float") &&
		m_extensions.isExtensionSupported("GL_ARB_shader_objects");

	if (!lIsExtSupported)
		return FF_FAIL;

	const GLubyte * glslInfo = glGetString(GL_SHADING_LANGUAGE_VERSION);

	int isCompiled = m_shader.Compile(vertexShaderCode.c_str(), fragmentShaderCode.c_str());

	if (isCompiled != 1)
		return FF_FAIL;

	this->fbos = CreateFBO(vp);
	if (this->fbos == nullptr)
		return FF_FAIL;

	/*
	m_extensions.glGenFramebuffersEXT(1, &frameBufferId);
	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBufferId);


	glGenTextures(1, &renderedTexture);
	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, vp->width, vp->height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	m_extensions.glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, renderedTexture, 0);

	GLenum status = m_extensions.glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
		return FF_FAIL;


	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	*/

	CreateFieldTexture(vp->width, vp->height, 0);

	m_shader.BindShader();


	//m_inputTextureLocation = m_shader.FindUniform("inputTexture");	
	m_inputTextureLocation = m_shader.FindUniform("texture0");
	m_extensions.glUniform1iARB(m_inputTextureLocation, 0);

	m_inputTextureLocation = m_shader.FindUniform("texture1");
	m_extensions.glUniform1iARB(m_inputTextureLocation, 1);

	m_inputTextureLocation = m_shader.FindUniform("texture2");
	m_extensions.glUniform1iARB(m_inputTextureLocation, 2);
	

	


	//the 0 means that the 'inputTexture' in
	//the shader will use the texture bound to GL texture unit 0
	//static float red[4] = { 0.0f, 1.0f, 0.0f, 1.0f };

	
	//m_extensions.glUniform4fvARB (m_inputTextureLocation, 1, red);

	m_shader.UnbindShader();


	
	


	return FF_SUCCESS;
}

DWORD FFGLFlows::DeInitGL()
{
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
	{
		this->CreateTextures(this->noiseTexturesDimension, this->noiseTexturesDimension, this->noiseTexturesAmount);
	}


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

#define SHADER_PROC
	// set rendering destination to FBO
	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, dstRenderBuffer);
	glClear(GL_COLOR_BUFFER_BIT);
	m_shader.BindShader();

	
	GLuint paramId = m_shader.FindUniform("dx");
	m_extensions.glUniform1fARB(paramId, 1.0 / (float)Texture.Width);

	paramId = m_shader.FindUniform("dy");
	m_extensions.glUniform1fARB(paramId, 1.0 / (float)Texture.Height);

	paramId = m_shader.FindUniform("alpha");
	m_extensions.glUniform1fARB(paramId, alphaNoisesTexture);

	paramId = m_shader.FindUniform("velocity");
	m_extensions.glUniform1fARB(paramId, this->velocity );

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
	m_extensions.glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
	m_extensions.glMultiTexCoord2f(GL_TEXTURE1, 0, 0);
	m_extensions.glMultiTexCoord2f(GL_TEXTURE2, 0, 0);
	glVertex2f(-1, -1);

	//upper left
	m_extensions.glMultiTexCoord2f(GL_TEXTURE0, 0, maxCoords.t);
	m_extensions.glMultiTexCoord2f(GL_TEXTURE1, 0, maxCoords.t);
	m_extensions.glMultiTexCoord2f(GL_TEXTURE2, 0, maxCoords.t);
	glVertex2f(-1, 1);

	//upper right
	m_extensions.glMultiTexCoord2f(GL_TEXTURE0, maxCoords.s, maxCoords.t);
	m_extensions.glMultiTexCoord2f(GL_TEXTURE1, maxCoords.s, maxCoords.t);
	m_extensions.glMultiTexCoord2f(GL_TEXTURE2, maxCoords.s, maxCoords.t);
	glVertex2f(1, 1);

	//lower right
	m_extensions.glMultiTexCoord2f(GL_TEXTURE0, maxCoords.s, 0);
	m_extensions.glMultiTexCoord2f(GL_TEXTURE1, maxCoords.s, 0);
	m_extensions.glMultiTexCoord2f(GL_TEXTURE2, maxCoords.s, 0);
	glVertex2f(1, -1);
	glEnd();
	//unbind the input texture
	glBindTexture(GL_TEXTURE_2D, 0);
	m_shader.UnbindShader();
	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, systemCurrentFbo);
#undef SHADER_PROC

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

	/*
	FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

	if (this->mainTextureId == NULL)
	{
		this->mainTextureId = new GLuint[1];
		glGenTextures(1, this->mainTextureId);
		glBindTexture(GL_TEXTURE_2D, *(this->mainTextureId));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, Texture.Width, Texture.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}

	

	//enable texturemapping
	glEnable(GL_TEXTURE_2D);


	//get the max s,t that correspond to the
	//width,height of the used portion of the allocated texture space
	FFGLTexCoords maxCoords = GetMaxGLTexCoords(Texture);




	glMatrixMode(GL_PROJECTION);

	// Saving the projection matrix
	glPushMatrix();
	glLoadIdentity();



	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, *(this->mainTextureId));

	glBegin(GL_QUADS);

	float deltha = -0.005;

	int HorisontalDimension = 100;
	int VerticalDimension = 100;

	float hDelta = 2.0 / (HorisontalDimension - 1);
	float vDelta = 2.0 / (VerticalDimension - 1);

	float sDelta = 1.0 / (HorisontalDimension - 1);
	float tDelta = 1.0 / (VerticalDimension - 1);

	for (int i = 0; i < VerticalDimension; i++)
	{
		for (int j = 0; j < HorisontalDimension; j++)
		{
			float xCoord = -1.0 + ((float)j) * hDelta;
			float yCoord = -1.0 + ((float)i) * vDelta;

			float sCoord = ((float)j) * sDelta - 0.1 * (this->velocity - 0.5)  * sinf(yCoord * 10.0 * (this->yFactor - this->yShift));
			float tCoord = ((float)i) * tDelta + 0.1 * (this->velocity - 0.5) * sinf(1.75*xCoord * 10.0 * (this->xFactor - this->xShift));

			//lower left
			glTexCoord2d(sCoord, tCoord);
			glVertex3f(xCoord, yCoord, 0);

			//upper left
			glTexCoord2d(sCoord, tCoord + tDelta);
			glVertex3f(xCoord, yCoord + vDelta, 0);

			//upper right
			glTexCoord2d(sCoord + sDelta, tCoord + tDelta);
			glVertex3f(xCoord + hDelta, yCoord + vDelta, 0);

			//lower right
			glTexCoord2d(sCoord + sDelta, tCoord);
			glVertex3f(xCoord + hDelta, yCoord, 0);
		}
	}

	glEnd();

	glColor4f(1, 1, 1, 0.3*this->alphaNoisesTexture);

	//bind the texture handle
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindTexture(GL_TEXTURE_2D, this->noiseTexturesIds[iCounter]);



	glBegin(GL_QUADS);

	//lower left
	glTexCoord2d(0.0, 0.0);
	glVertex3f(-1, -1, 0.1);

	//upper left
	glTexCoord2d(0.0, maxCoords.t);
	glVertex3f(-1, 1, 0.1);

	//upper right
	glTexCoord2d(maxCoords.s, maxCoords.t);
	glVertex3f(1, 1, 0.1);

	//lower right
	glTexCoord2d(maxCoords.s, 0.0);
	glVertex3f(1, -1, 0.1);

	glEnd();



	glColor4f(1, 1, 1, 0.3*this->alphaImageTexture);
	glBindTexture(GL_TEXTURE_2D, Texture.Handle);


	glBegin(GL_QUADS);

	//lower left
	glTexCoord2d(0.0, 0.0);
	glVertex3f(-1, -1, 0.11);

	//upper left
	glTexCoord2d(0.0, maxCoords.t);
	glVertex3f(-1, 1, 0.1);

	//upper right
	glTexCoord2d(maxCoords.s, maxCoords.t);
	glVertex3f(1, 1, 0.1);

	//lower right
	glTexCoord2d(maxCoords.s, 0.0);
	glVertex3f(1, -1, 0.1);

	glEnd();

	//disable blending
	glDisable(GL_BLEND);

	this->iCounter++;
	this->iCounter %= this->noiseTexturesAmount;


	if (this->mainTextureId != NULL)
	{
		glBindTexture(GL_TEXTURE_2D, *(this->mainTextureId));
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, Texture.Width, Texture.Height, 0);
		GLenum resultCode = glGetError();
		GLint dfdf = resultCode;
	}
	*/

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
	this->iCounter %= this->noiseTexturesAmount;

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

	GLenum errCode;

	for (k = 0; k < texNum; k++)
	{
		t = k * 256 / texNum;
		for (i = 0; i < width*height; i++)
			pat[i] = lut[(t + phase[i]) % 255];

		glBindTexture(GL_TEXTURE_2D, this->noiseTexturesIds[k]);
		errCode = glGetError();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		errCode = glGetError();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		errCode = glGetError();

		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pat);
		errCode = glGetError();
		if (errCode != 0)
			errCode = glGetError();

	}

	delete phase;
	delete pat;

}

void FFGLFlows::CreateFieldTexture(int width, int height, float param)
{
	if (fieldTextureId == 0)
	{
		glGenTextures(1, &fieldTextureId);
	}
	else
	{
		glDeleteTextures(1, &fieldTextureId);
		glGenTextures(1, &fieldTextureId);
	}

	float * fieldData = new float[width*height * 3];
	bool isFirst = true;

	
	float hDelta = 2.0 / (width-1);
	float vDelta = 2.0 / (height-1);

	
	for (int i = 0; i < height; i++)
	{
		int offset = 3 * i * width;
		for (int j = 0; j < width; j++)
		{	
			/*
			float xCoord = -1.0 + ((float)j) * hDelta;
			float yCoord = -1.0 + ((float)i) * vDelta;

			float sCoord = ((float)j) * sDelta - 0.1 * (this->velocity - 0.5)  * sinf(yCoord * 10.0 * (this->yFactor - this->yShift));
			float tCoord = ((float)i) * tDelta + 0.1 * (this->velocity - 0.5) * sinf(1.75*xCoord * 10.0 * (this->xFactor - this->xShift));
			*/

			float x = -1.0 + ((float)j) * hDelta;
			float y = -1.0 + ((float)i) * vDelta;


			fieldData[offset + 0] = cos(y);//sCoord;
			fieldData[offset + 1] = sin(x);//tCoord;
			fieldData[offset + 2] = 0.0;

			/*
			if (isFirst)
			{
				fieldData[offset + 0] = 1;
				fieldData[offset + 1] = 1;
				fieldData[offset + 2] = 1;
				isFirst = false;
			}
			else
			{
				
			}*/
			offset += 3;
		}
	}

	/*
	fieldData[width*height * 3 - 1] = 1;
	fieldData[width*height * 3 - 2] = 1;
	fieldData[width*height * 3 - 3] = 1;
	*/

	glBindTexture(GL_TEXTURE_2D, fieldTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, width, height, 0, GL_RGB, GL_FLOAT, fieldData);

	GLenum error = glGetError();
	
	//unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] fieldData;
}

DWORD FFGLFlows::GetParameter(DWORD dwIndex)
{
	DWORD dwRet;

	switch (dwIndex) {

	case FFPARAM_VALUE1_NOISE_BLENDING_FACTOR:
		*((float *)(unsigned)(&dwRet)) = this->alphaNoisesTexture;
		return dwRet;

	case FFPARAM_VALUE2_TEXTURE_BLENDING_FACTOR:
		*((float *)(unsigned)(&dwRet)) = this->alphaImageTexture;
		return dwRet;

	case FFPARAM_VALUE3_NOISE_TEXTURES_AMOUNT:
		*((float *)(unsigned)(&dwRet)) = ((float)this->noiseTexturesAmount) / ((float)this->maxNoiseTexturesAmount);
		return dwRet;

	case FFPARAM_VALUE4_NOISE_TEXTURES_DIMENSION:
		*((float *)(unsigned)(&dwRet)) = ((float)this->noiseTexturesDimension) / ((float)this->maxNoiseTexturesDimension);
		return dwRet;

	case FFPARAM_VALUE5_XFACTOR:
		*((float *)(unsigned)(&dwRet)) = this->xFactor;
		return dwRet;

	case FFPARAM_VALUE6_YFACTOR:
		*((float *)(unsigned)(&dwRet)) = this->yFactor;
		return dwRet;

	case FFPARAM_VALUE_VELOCITY:
		*((float *)(unsigned)(&dwRet)) = this->velocity;
		return dwRet;

	case FFPARAM_VALUE_VELOCITY_SCALE:
		*((float *)(unsigned)(&dwRet)) = this->velocityScale;
		return dwRet;

	case FFPARAM_VALUE_XSHIFT:
		*((float *)(unsigned)(&dwRet)) = this->xShift;
		return dwRet;

	case FFPARAM_VALUE_YSHIFT:
		*((float *)(unsigned)(&dwRet)) = this->yShift;
		return dwRet;

		/*
	case FFPARAM_VALUE2_SCALE  :
	//sizeof(DWORD) must == sizeof(float)


	case FFPARAM_VALUE3_RX:
	//sizeof(DWORD) must == sizeof(float)
	*((float *)(unsigned)(&dwRet)) = this->fXAngle;
	return dwRet;

	case FFPARAM_VALUE4_RY:
	//sizeof(DWORD) must == sizeof(float)
	*((float *)(unsigned)(&dwRet)) = this->fYAngle;
	return dwRet;

	case FFPARAM_VALUE5_DISTANCE:
	//sizeof(DWORD) must == sizeof(float)
	*((float *)(unsigned)(&dwRet)) = this->fDistance;
	return dwRet;

	case FFPARAM_VALUE6_PCOUNT:
	//sizeof(DWORD) must == sizeof(float)
	*((float *)(unsigned)(&dwRet)) = this->fPlanesCount;
	return dwRet;

	case FFPARAM_VALUE7_FOVY:
	//sizeof(DWORD) must == sizeof(float)
	*((float *)(unsigned)(&dwRet)) = this->fAngle;
	return dwRet;
	*/
	default:
		return FF_FAIL;
	}
}

DWORD FFGLFlows::SetParameter(const SetParameterStruct* pParam)
{

	if (pParam != NULL)
	{
		bool isFieldChanged = false;

		float fNewValue = *((float *)(unsigned)&(pParam->NewParameterValue));

		switch (pParam->ParameterNumber)
		{


		case FFPARAM_VALUE1_NOISE_BLENDING_FACTOR:
			this->alphaNoisesTexture = fNewValue;
			break;

		case FFPARAM_VALUE2_TEXTURE_BLENDING_FACTOR:
			this->alphaImageTexture = fNewValue;
			break;

		case FFPARAM_VALUE3_NOISE_TEXTURES_AMOUNT:
		{
			int newAmount = fNewValue * (float)this->maxNoiseTexturesAmount;
			newAmount = newAmount == 0 ? 1 : newAmount;
			if (newAmount != this->noiseTexturesAmount)
			{
				if (this->noiseTexturesIds != NULL)
				{
					glDeleteTextures(this->noiseTexturesAmount, this->noiseTexturesIds);
					delete this->noiseTexturesIds;
					this->noiseTexturesIds = NULL;
				}

				this->noiseTexturesAmount = newAmount;
			};
		};
		break;

		case FFPARAM_VALUE4_NOISE_TEXTURES_DIMENSION:
		{
			int newDimension = fNewValue * (float)this->maxNoiseTexturesDimension;
			newDimension = newDimension == 0 ? 1 : newDimension;
			if (newDimension != this->noiseTexturesDimension)
			{
				if (this->noiseTexturesIds != NULL)
				{
					glDeleteTextures(this->noiseTexturesAmount, this->noiseTexturesIds);
					delete this->noiseTexturesIds;
					this->noiseTexturesIds = NULL;
				}

				this->noiseTexturesDimension = newDimension;
			};
		};
		break;


		case FFPARAM_VALUE5_XFACTOR:
		{
			isFieldChanged = (this->xFactor != fNewValue);
			this->xFactor = fNewValue;
		};
		break;

		case FFPARAM_VALUE6_YFACTOR:
		{
			isFieldChanged = (this->yFactor != fNewValue);
			this->yFactor = fNewValue;
		};
		break;

		case FFPARAM_VALUE_VELOCITY:
		{
			this->velocity = fNewValue;
		};
		break;

		case FFPARAM_VALUE_VELOCITY_SCALE:
		{
			this->velocityScale  = fNewValue;
		};
		break;

		case FFPARAM_VALUE_XSHIFT:
		{
			isFieldChanged = (this->xShift != fNewValue);
			this->xShift = fNewValue;
		};
		break;

		case FFPARAM_VALUE_YSHIFT:
		{
			isFieldChanged = (this->yShift != fNewValue);
			this->yShift = fNewValue;
		};
		break;



			/*
			case FFPARAM_VALUE3_RX:
			this->fXAngle = fNewValue;
			break;

			case FFPARAM_VALUE4_RY:
			this->fYAngle = fNewValue;
			break;

			case FFPARAM_VALUE5_DISTANCE:
			if (this->fDistance != fNewValue)
			{
			this->fDistance = fNewValue;
			this->isGeometryRebuildNeeded  = true;
			};
			break;

			case FFPARAM_VALUE6_PCOUNT:
			if (this->fPlanesCount != fNewValue)
			{
			this->fPlanesCount = fNewValue;
			this->iPlanesCount = (float)MaxPlanes * this->fPlanesCount;
			if (this->iPlanesCount == 0)
			this->iPlanesCount = 1;
			this->isGeometryRebuildNeeded = true;
			};
			break;

			case FFPARAM_VALUE7_FOVY:
			this->fAngle = fNewValue;
			break;
			*/
		default:
			return FF_FAIL;
		}

		if (isFieldChanged)
		{
			CreateFieldTexture(800, 600, 0);
			isFieldChanged = false;
		}

		return FF_SUCCESS;

	}

	return FF_FAIL;
}
