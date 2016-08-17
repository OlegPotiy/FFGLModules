#ifndef FFGLFLOW_H
#define FFGLFLOW_H

#include "../../FFGL/FFGLShader.h"
#include "../../FFGL/FFGLPluginSDK.h"
#include "../../FFGL/FFGLLib.h"

#include <string>
#include <math.h>
#include <map>
#include <memory>
#include <vector>


#define STRINGIFY( expr ) new std::string(#expr)
#define mulFtoI(v1, v2) (int)(((float)(v1))*((float)(v2)))

/* 
	Parameters codes
*/
enum class ParamNames : int
{
	FUNC_DEF,
	NOISE_FACTOR,
	IMG_BLENDING_FACTOR,
	NOISES_TEXTURES_COUNT,
	NOISES_SCALE,
	OPERATOR_TYPE,	
	XFACTOR,
	YFACTOR,
	XSHIFT,
	YSHIFT,
	VELOCITY,
	VELOCITY_SCALE
	
};

struct ParamDefinition
{
public:
	std::string paramName;
	DWORD		paramType;
	float*		floatValueStorage;		
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

	std::string* fscUniform;
	std::string* fscAlnalyticStart;
	std::string* fscAlnalyticEnd;
	std::string* fscSobel;
	std::string* fscDirect;
	std::string* fscAdvectionFunc;

public:
	FBOPair* CreateFBO(const FFGLViewportStruct *vp);

private:

	

	FFGLExtensions m_extensions;

	FFGLShader analyticShader;
	FFGLShader sobelShader;
	FFGLShader directShader;

	std::map<int, ParamDefinition> parameterDefinitions;

		



	GLfloat alphaNoisesTexture{ 0.5f };
	GLfloat alphaImageTexture{ 0.5f };

	float xFactor{ 0.1f };
	float yFactor{ 0.1f };
	float xShift{ 0.0f };
	float yShift{ 0.0f };

	float velocity{ 0.5f };
	float velocityScale{ 0.5f };
	float noiseDimScale{ 0.5f };
	float noiseTexturesCountFactor{ 0.5f };
	float operatorTypeFactor{ 0.5 };
	
	std::string fieldCode{ "vec2(0.1,y)"};


	GLuint noiseWidth;
	
	GLuint noiseHeight;

	
	
	
	int currentTexId{ 0 };
	std::vector<GLuint> texIds;


	bool IsFirstFrame = true;


	GLuint frameBufferId;
	GLuint renderedTexture;


	void DeleteNoiseTextures();	
	void EnsureTexturesCreated();

};



#endif
