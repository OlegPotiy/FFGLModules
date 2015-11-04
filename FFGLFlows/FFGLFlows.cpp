#include "../FFGL Lib/FFGL.h"
#include "../FFGL Lib/FFGLLib.h"
#include "FFGLFlows.h"
#include <gl\GLU.h>
#include <math.h>
#define USE_VBO

// Parameters
#define FFPARAM_VALUE1_NOISE_BLENDING_FACTOR	(0)
#define FFPARAM_VALUE2_TEXTURE_BLENDING_FACTOR	(1)
#define FFPARAM_VALUE3_NOISE_TEXTURES_AMOUNT	(2)
#define FFPARAM_VALUE4_NOISE_TEXTURES_DIMENSION	(3)
#define FFPARAM_VALUE5_XFACTOR	(4)
#define FFPARAM_VALUE6_YFACTOR	(5)
#define FFPARAM_VALUE7_VELOCITY	(6)
#define FFPARAM_VALUE8_XSHIFT	(7)
#define FFPARAM_VALUE9_YSHIFT	(8)


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

	SetParamInfo(FFPARAM_VALUE1_NOISE_BLENDING_FACTOR, "Noise fraction", FF_TYPE_STANDARD, this->alphaNoisesTexture );
	SetParamInfo(FFPARAM_VALUE2_TEXTURE_BLENDING_FACTOR, "Img fraction", FF_TYPE_STANDARD, this->alphaImageTexture );
	SetParamInfo(FFPARAM_VALUE3_NOISE_TEXTURES_AMOUNT, "Noises amount", FF_TYPE_STANDARD, ((float)this->noiseTexturesAmount)/((float)this->maxNoiseTexturesAmount));
	SetParamInfo(FFPARAM_VALUE4_NOISE_TEXTURES_DIMENSION, "Noises dim", FF_TYPE_STANDARD, ((float)this->noiseTexturesDimension) / ((float)this->maxNoiseTexturesDimension));

	SetParamInfo(FFPARAM_VALUE5_XFACTOR, "X Factor", FF_TYPE_STANDARD, this->xFactor);
	SetParamInfo(FFPARAM_VALUE6_YFACTOR, "Y Factor", FF_TYPE_STANDARD, this->yFactor);
	SetParamInfo(FFPARAM_VALUE7_VELOCITY, "Velosity", FF_TYPE_STANDARD, this->velocity);
	SetParamInfo(FFPARAM_VALUE8_XSHIFT, "X Shift", FF_TYPE_STANDARD, this->xShift);
	SetParamInfo(FFPARAM_VALUE9_YSHIFT, "Y Shift", FF_TYPE_STANDARD, this->yShift);
}

FFGLFlows::~FFGLFlows()
{
}

DWORD FFGLFlows::InitGL(const FFGLViewportStruct *vp)
{
	



#ifndef  USE_VBO
	return FF_SUCCESS;
#else
	m_extensions.Initialize();
	bool lIsExtSupported =	m_extensions.isExtensionSupported("GL_ARB_vertex_buffer_object") && 
							m_extensions.isExtensionSupported("GL_ARB_texture_float");


	return lIsExtSupported ? FF_SUCCESS : FF_FAIL;
#endif
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

	if (this->mainTextureId == NULL)
	{
		this->mainTextureId = new GLuint[1];
		glGenTextures(1, this->mainTextureId);
		glBindTexture(GL_TEXTURE_2D, *(this->mainTextureId));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, Texture.Width, Texture.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}

	if (this->noiseTexturesIds == NULL)
	{
		this->CreateTextures(this->noiseTexturesDimension, this->noiseTexturesDimension, this->noiseTexturesAmount);
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


	glMatrixMode(GL_PROJECTION);
	//Restoring projection matrix
	glPopMatrix();

	//unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	//disable texturemapping
	glDisable(GL_TEXTURE_2D);



	//restore default color
	glColor4f(1.f, 1.f, 1.f, 1.f);

	return FF_SUCCESS;
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

	case FFPARAM_VALUE7_VELOCITY:
		*((float *)(unsigned)(&dwRet)) = this->velocity;
		return dwRet;

	case FFPARAM_VALUE8_XSHIFT:
		*((float *)(unsigned)(&dwRet)) = this->xShift;
	return dwRet;

	case FFPARAM_VALUE9_YSHIFT:
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
			this->xFactor = fNewValue;
		break;

		case FFPARAM_VALUE6_YFACTOR:
			this->yFactor = fNewValue;
		break;

		case FFPARAM_VALUE7_VELOCITY:
			this->velocity = fNewValue;
		break;

		case FFPARAM_VALUE8_XSHIFT:
			this->xShift = fNewValue;
		break;

		case FFPARAM_VALUE9_YSHIFT:
			this->yShift = fNewValue;
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

		return FF_SUCCESS;

	}

	return FF_FAIL;
}
