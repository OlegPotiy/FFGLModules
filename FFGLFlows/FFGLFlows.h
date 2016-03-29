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


	FBOPair* CreateFBO(const FFGLViewportStruct *vp);

	void CreateTextures(int width, int height, int texNum);
	void CreateFieldTexture(int width, int height, float param);


	FFGLExtensions m_extensions;
	FFGLShader m_shader;

	struct GLVertexTriplet { GLfloat x, y, z; };
	struct GLTexcoords { GLfloat s, t; };

	GLfloat alphaNoisesTexture = 0.5;
	GLfloat alphaImageTexture = 0.5;

	float xFactor = 0.1;
	float yFactor = 0.1;
	float velocity = 0.6;
	float velocityScale = 0.6;
	float xShift = 0.0;
	float yShift = 0.0;

	int noiseTexturesDimension = 256;
	const int maxNoiseTexturesDimension = 1024;

	int noiseTexturesAmount = 32;
	const int maxNoiseTexturesAmount = 256;

	int iCounter = 0;

	GLuint *mainTextureId = nullptr;
	GLuint *noiseTexturesIds = nullptr;
	GLuint fieldTextureId = 0;

	
	bool IsFirstFrame = true;


	GLuint frameBufferId;
	GLuint renderedTexture;

	GLint m_inputTextureLocation;
	GLint m_heatTextureLocation;
	GLint m_maxCoordsLocation;
	GLint m_heatAmountLocation;

};



#endif
