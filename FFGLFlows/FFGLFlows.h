#ifndef FFGLBRIGHTNESS_H
#define FFGLBRIGHTNESS_H


#include "../FFGL Lib/FFGLShader.h"
#include "../FFGL Lib/FFGLPluginSDK.h"

#define STRINGIFY( expr ) std::string(#expr)

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



public:
	FBOPair* CreateFBO(const FFGLViewportStruct *vp);

private:
	
	FFGLExtensions m_extensions;
	FFGLShader m_shader;


	GLfloat alphaNoisesTexture = 0.5f;
	GLfloat alphaImageTexture = 0.5f;

	float xFactor = 0.1f;
	float yFactor = 0.1f;
	float velocity = 0.6f;
	float velocityScale = 0.6f;
	float xShift = 0.0f;
	float yShift = 0.0f;
	float noiseDimScale = 0.5f;


	GLuint maxHorisontalNoiseDim;
	GLuint maxVerticalNoiseDim;

	int noiseTexturesAmount = 32;
	const int maxNoiseTexturesAmount = 256;

	int iCounter = 0;

	GLuint *mainTextureId = nullptr;
	GLuint *noiseTexturesIds = nullptr;
	GLuint fieldTextureId = 0;

	
	bool IsFirstFrame = true;


	GLuint frameBufferId;
	GLuint renderedTexture;


	
	


	void CreateTextures(int width, int height, int texNum);
	void CreateFieldTexture(int width, int height, float param);


};



#endif
