#ifndef __GLSKYBOX_RENDERER_H__
#define __GLSKYBOX_RENDERER_H__

#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
#include <jcGLframework/GLTexture.h>

#include <jcCommonFramework/ResourceString.h>


class GLSkyboxRenderer
{
	GLTexture envMap_;
	GLTexture envMapIrradiance_;
	GLTexture brdfLUT_ = { GL_TEXTURE_2D, appendToRoot ( "assets/images/brdfLUT.ktx" ) };
	GLShader shdCubeVertex_ = GLShader { appendToRoot ( "assets/shaders/GL10_cube.vert" ).c_str () };
	GLShader shdCubeFragment_ = GLShader { appendToRoot ( "assets/shaders/GL10_cube.frag" ).c_str () };
	GLProgram progCube_ = GLProgram { shdCubeVertex_, shdCubeFragment_ };
	GLuint dummyVAO_;

public:
	GLSkyboxRenderer ( const char* envMap = appendToRoot ( "assets/images/immenstadter_horn_2k.hdr" ).c_str (),
		const char* envMapIrradiance = appendToRoot ( "assets/images/immenstadter_horn_2k_irradiance.hdr" ).c_str () )
		: envMap_ ( GL_TEXTURE_CUBE_MAP, envMap )
		, envMapIrradiance_ ( GL_TEXTURE_CUBE_MAP, envMapIrradiance )
	{
		glCreateVertexArrays ( 1, &dummyVAO_ );
		const GLuint pbrTextures [] = { envMap_.getHandle (), envMapIrradiance_.getHandle (), brdfLUT_.getHandle () };
		// binding points for data/shaders/PBR.sp
		glBindTextures ( 5, 3, pbrTextures );
	}

	~GLSkyboxRenderer ()
	{
		glDeleteVertexArrays ( 1, &dummyVAO_ );
	}

	void draw ()
	{
		progCube_.useProgram ();
		glBindTextureUnit ( 1, envMap_.getHandle () );
		glDepthMask ( false );
		glBindVertexArray ( dummyVAO_ );
		glDrawArrays ( GL_TRIANGLES, 0, 36 );
		glDepthMask ( true );
	}
};

#endif // !__GLSKYBOX_RENDERER_H__
