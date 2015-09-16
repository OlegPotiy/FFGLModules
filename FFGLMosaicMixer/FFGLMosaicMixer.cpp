#include "../FFGL Lib/FFGL.h"
#include "../FFGL Lib/FFGLLib.h"
#include "FFGLMosaicMixer.h"
#include <gl\GLU.h>
#include <math.h>
#include "GLCommons.h"

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

	this->BuildGeometry(100);


	topNode = new GLQuadNode();

	
	GLQuadNode * tmpNode = NULL;
	
	
	tmpNode = new GLQuadNode();
	topNode->SetChild(GLQuadElems::UPPER_RIGHT, tmpNode);

}

FFGLMosaicMixer::~FFGLMosaicMixer()
{
}

void FFGLMosaicMixer::BuildGeometry(int cnt)
{
	objCount = cnt;

	if (this->quads != NULL)
		delete[] this->quads;

	this->quads = new GLQuad[this->objCount];

	
	GLVertex invPoint;
	GLVertex centerPoint;
	GLVertex centerTexPoint;
	for (int i = 0; i < this->objCount; i++)
	{	
		invPoint.x = (float)rand() / (float)(RAND_MAX + 1);
		invPoint.y = (float)rand() / (float)(RAND_MAX + 1);
		invPoint.z = .5 * (float)i / (float) this->objCount;

		centerPoint.x = 2.0 * invPoint.x - 1.0;
		centerPoint.y = 2.0 * invPoint.y - 1.0;
		centerPoint.z = 2.0 * invPoint.z - 1.0;

		float halphSize = .25 * (float)rand() / (float)(RAND_MAX + 1);
		float quaterSize = .5 * halphSize;

		this->quads[i].verts[0].x = centerPoint.x - halphSize;
		this->quads[i].verts[0].y = centerPoint.y - halphSize;
		this->quads[i].verts[0].z = centerPoint.z;

		this->quads[i].verts[1].x = centerPoint.x + halphSize;
		this->quads[i].verts[1].y = centerPoint.y - halphSize;
		this->quads[i].verts[1].z = centerPoint.z;

		this->quads[i].verts[2].x = centerPoint.x + halphSize;
		this->quads[i].verts[2].y = centerPoint.y + halphSize;
		this->quads[i].verts[2].z = centerPoint.z;

		this->quads[i].verts[3].x = centerPoint.x - halphSize;
		this->quads[i].verts[3].y = centerPoint.y + halphSize;
		this->quads[i].verts[3].z = centerPoint.z;

		

		this->quads[i].texcoords[0].s = invPoint.x - quaterSize;
		this->quads[i].texcoords[0].t = invPoint.y - quaterSize;

		this->quads[i].texcoords[1].s = invPoint.x + quaterSize;
		this->quads[i].texcoords[1].t = invPoint.y - quaterSize;

		this->quads[i].texcoords[2].s = invPoint.x + quaterSize;
		this->quads[i].texcoords[2].t = invPoint.y + quaterSize;

		this->quads[i].texcoords[3].s = invPoint.x - quaterSize;
		this->quads[i].texcoords[3].t = invPoint.y + quaterSize;

	}
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
	
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);


	// Top central triangle
	glBindTexture(GL_TEXTURE_2D, TextureObject1.Handle);

	glBegin(GL_QUADS);

	//lower left
	glTexCoord2d(0, 0);
	glVertex3f(-1, -1, 0);

	//lower right
	glTexCoord2d(1, 0);
	glVertex3f(1, -1, 0);


	//upper right
	glTexCoord2d(1, 1);
	glVertex3f(1, 1, 0);

	//upper left
	glTexCoord2d(0, 1);
	glVertex3f(-1, 1, 0);

	glEnd();

	glBindTexture(GL_TEXTURE_2D, TextureObject2.Handle);

	if (m_blend < 1)
	{
		for (int i = 0; i < this->objCount * m_blend; i++)
		{
			glBegin(GL_QUADS);

			//lower left
			glTexCoord2d(this->quads[i].texcoords[0].s, this->quads[i].texcoords[0].t);
			glVertex3f(this->quads[i].verts[0].x, this->quads[i].verts[0].y, this->quads[i].verts[0].z);
			//glVertex3f(0, 0, 0);

			//lower right
			glTexCoord2d(this->quads[i].texcoords[1].s, this->quads[i].texcoords[1].t);
			glVertex3f(this->quads[i].verts[1].x, this->quads[i].verts[1].y, this->quads[i].verts[1].z);
			//glVertex3f(1, 0, 0);


			//upper right
			glTexCoord2d(this->quads[i].texcoords[2].s, this->quads[i].texcoords[2].t);
			glVertex3f(this->quads[i].verts[2].x, this->quads[i].verts[2].y, this->quads[i].verts[2].z);
			//glVertex3f(1, 1, 0);

			//upper left
			glTexCoord2d(this->quads[i].texcoords[3].s, this->quads[i].texcoords[3].t);
			glVertex3f(this->quads[i].verts[3].x, this->quads[i].verts[3].y, this->quads[i].verts[3].z);
			//glVertex3f(0, 1, 0);

			glEnd();
		}
	}
	else
	{
		glBegin(GL_QUADS);

		//lower left
		glTexCoord2d(0, 0);
		glVertex3f(-1, -1, 0);

		//lower right
		glTexCoord2d(1, 0);
		glVertex3f(1, -1, 0);


		//upper right
		glTexCoord2d(1, 1);
		glVertex3f(1, 1, 0);

		//upper left
		glTexCoord2d(0, 1);
		glVertex3f(-1, 1, 0);

		glEnd();
	}



	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

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