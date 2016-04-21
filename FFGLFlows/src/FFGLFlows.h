#ifndef FFGLFLOW_H
#define FFGLFLOW_H

#include "../../FFGL/FFGLShader.h"
#include "../../FFGL/FFGLPluginSDK.h"
#include "../../FFGL/FFGLLib.h"

#include <string>
#include <math.h>
#include <map>


#define STRINGIFY( expr ) new std::string(#expr)
#define mulFtoI(v1, v2) (int)(((float)(v1))*((float)(v2)))

enum class ParamNames : int
{
	NOISE_FACTOR,
	IMG_BLENDING_FACTOR,
	NOISES_TEXTURES_COUNT,
	NOISES_SCALE,
	XFACTOR,
	YFACTOR,
	XSHIFT,
	YSHIFT,
	VELOCITY,
	VELOCITY_SCALE,
};

struct ParamDefinition
{
public:
	std::string paramName;
	DWORD		paramType;
	float*		paramDefaultValuePtr;
};


class FFGLFlows : public CFreeFrameGLPlugin
{
public:

	FFGLFlows();
	virtual ~FFGLFlows();

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
		*ppInstance = new FFGLFlows();
		if (*ppInstance != NULL) return FF_SUCCESS;
		return FF_FAIL;
	}

protected:



private:

	class FBOPair 
	{
	public:
		GLuint evenBufferId;
		GLuint oddBufferId;

		GLuint evenTextureId;
		GLuint oddTextureId;
		
		bool isEven;

	} *fbos;


	std::string* vertexShaderCode;
	std::string* fragmentShaderCode;

public:
	FBOPair* CreateFBO(const FFGLViewportStruct *vp);

private:
	
	FFGLExtensions m_extensions;
	FFGLShader m_shader;

	std::map<int, ParamDefinition>* parameterDefinitions;
	


	GLfloat alphaNoisesTexture{ 0.5f };
	GLfloat alphaImageTexture{ 0.5f };

	float xFactor{ 0.1f };
	float yFactor{ 0.1f };
	float velocity{ 0.6f };
	float velocityScale{ 0.6f };
	float xShift{ 0.0f };
	float yShift{ 0.0f };
	float noiseDimScale = 0.5f;
	float noiseTexturesCountFactor = 0.5f;

	int ntexCount = mulFtoI(noiseTexturesCountFactor, maxNoiseTexturesAmount);

	float fieldSource { 0.5 };


	GLuint maxHorisontalNoiseDim;
	GLuint maxVerticalNoiseDim;

	
	const int maxNoiseTexturesAmount = 256;

	int iCounter = 0;

	GLuint *mainTextureId = nullptr;
	GLuint *noiseTexturesIds = nullptr;
	GLuint fieldTextureId = 0;

	
	bool IsFirstFrame = true;


	GLuint frameBufferId;
	GLuint renderedTexture;


	
	

	void DeleteNoiseTextures();
	void CreateTextures(int width, int height, int texNum);
	
};



#endif
