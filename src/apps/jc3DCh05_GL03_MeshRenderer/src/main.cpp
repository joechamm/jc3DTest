#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "GLShader.h"
#include "UtilsMath.h"
#include "debug.h"
#include "Camera.h"
#include "VtxData.h"

#include <vector>

#include <helpers/RootDir.h>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;
using glm::ivec2;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

struct MouseState
{
	glm::vec2 pos = glm::vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

CameraPositioner_FirstPerson positioner ( vec3 ( -31.5f, 7.5f, -9.5f ), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
Camera camera ( positioner );

struct DrawElementsIndirectCommand
{
	GLuint count_;
	GLuint instanceCount_;
	GLuint firstIndex_;
	GLuint baseVertex_;
	GLuint baseInstance_;
};

/**
 * @brief GLMesh helper class constructor accepts pointers to the indices and vertices data buffers. 
 * The Data buffers are used as-is, and they are uploaded directly into the respective OpenGL buffers. 
 * The number of indices is inferred from the indices buffer size, assuming indices are stored as 32-bit unsigned integers.
*/
class GLMesh final
{
private:
	GLuint vao_;
	uint32_t numIndices_;
	GLBuffer bufferIndices_;
	GLBuffer bufferVertices_;

public:
	GLMesh ( const uint32_t* indices, uint32_t indicesSizeBytes, const float* vertexData, uint32_t verticesSizeBytes )
		: numIndices_ ( indicesSizeBytes / sizeof ( uint32_t ) )
		, bufferIndices_ ( indicesSizeBytes, indices, 0 )
		, bufferVertices_ ( verticesSizeBytes, vertexData, 0 )
	{
		glCreateVertexArrays ( 1, &vao_ );
		glVertexArrayElementBuffer ( vao_, bufferIndices_.getHandle () );
		glVertexArrayVertexBuffer ( vao_, 0, bufferVertices_.getHandle (), 0, sizeof ( vec3 ) );
		// the vertex data format for this recipe only contains vertices that are represented as vec3
		glEnableVertexArrayAttrib ( vao_, 0 );
		glVertexArrayAttribFormat ( vao_, 0, 3, GL_FLOAT, GL_FALSE, 0 );
		glVertexArrayAttribBinding ( vao_, 0, 0 );
	}

	~GLMesh ()
	{
		glDeleteVertexArrays ( 1, &vao_ );
	}

	void draw () const
	{
		glBindVertexArray ( vao_ );
		glDrawElements ( GL_TRIANGLES, static_cast<GLsizei>(numIndices_), GL_UNSIGNED_INT, nullptr );

	}
};

int main ( void )
{
	glfwSetErrorCallback (
		[]( int err, const char* description )
		{
			fprintf ( stderr, "Error: %s\n", description );
		}
	);

	if ( !glfwInit () )
		exit ( EXIT_FAILURE );

	glfwWindowHint ( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint ( GLFW_CONTEXT_VERSION_MINOR, 6 );
	glfwWindowHint ( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
#ifdef _DEBUG
	glfwWindowHint ( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );
#endif

	GLFWwindow* window = glfwCreateWindow ( 1280, 720, "OpenGL Camera example", nullptr, nullptr );
	if ( !window )
	{
		glfwTerminate ();
		exit ( EXIT_FAILURE );
	}

	/// SETUP callbacks
	glfwSetKeyCallback (
		window,
		[]( GLFWwindow* window, int key, int scancode, int action, int mods )
		{
			const bool press = action != GLFW_RELEASE;
			if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
			if ( key == GLFW_KEY_W ) positioner.movement_.forward_ = press;
			if ( key == GLFW_KEY_S ) positioner.movement_.backward_ = press;
			if ( key == GLFW_KEY_A ) positioner.movement_.left_ = press;
			if ( key == GLFW_KEY_D ) positioner.movement_.right_ = press;
			if ( key == GLFW_KEY_1 ) positioner.movement_.up_ = press;
			if ( key == GLFW_KEY_2 ) positioner.movement_.down_ = press;
			if ( mods & GLFW_MOD_SHIFT ) positioner.movement_.fastSpeed_ = press;
			if ( key == GLFW_KEY_SPACE ) positioner.setUpVector ( vec3 ( 0.0f, 1.0f, 0.0f ) );
		}
	);

	glfwSetCursorPosCallback ( window, []( auto* window, double x, double y ) {
		int width, height;
		glfwGetFramebufferSize ( window, &width, &height );
		mouseState.pos.x = static_cast<float>(x / width);
		mouseState.pos.y = static_cast<float>(y / height);
		}
	);

	glfwSetMouseButtonCallback ( window, []( auto* window, int button, int action, int mods ) {
		if ( button == GLFW_MOUSE_BUTTON_LEFT )
		{
			mouseState.pressedLeft = action == GLFW_PRESS;
		}
		}
	);


	glfwMakeContextCurrent ( window );
	gladLoadGL ( glfwGetProcAddress );
	glfwSwapInterval ( 0 );

	// setup debugging
	initDebug ();

	// load shaders
	std::string shdModelVtxFilename = ROOT_DIR + std::string ( "assets/shaders/GL04_duck.vert" );
	std::string shdModelFragFilename = ROOT_DIR + std::string ( "assets/shaders/GL04_duck.frag" );
	std::string shdCubeVtxFilename = ROOT_DIR + std::string ( "assets/shaders/GL04_cube.vert" );
	std::string shdCubeFragFilename = ROOT_DIR + std::string ( "assets/shaders/GL04_cube.frag" );

	GLShader shdModelVertex ( shdModelVtxFilename.c_str () );
	GLShader shdModelFragment ( shdModelFragFilename.c_str () );
	GLProgram progModel ( shdModelVertex, shdModelFragment );
	progModel.useProgram ();

	GLShader shdCubeVertex ( shdCubeVtxFilename.c_str () );
	GLShader shdCubeFragment ( shdCubeFragFilename.c_str () );
	GLProgram progCube ( shdCubeVertex, shdCubeFragment );

	// SETUP uniform buffer 
	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );

	GLuint perFrameDataBuffer;
	glCreateBuffers ( 1, &perFrameDataBuffer );
	glNamedBufferStorage ( perFrameDataBuffer, kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kUniformBufferSize );

	// Set the clear color and enable depth testing
	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	glEnable ( GL_DEPTH_TEST );

	// Use assimp to import our scene and convert any primitives to triangles
	std::string sceneFilename = ROOT_DIR + std::string ( "assets/models/rubber_duck/scene.gltf" );
	const aiScene* scene = aiImportFile ( sceneFilename.c_str (), aiProcess_Triangulate );

	// Error checking
	if ( !scene || !scene->HasMeshes () )
	{
		printf ( "Unable to load %s\n", sceneFilename.c_str () );
		exit ( 255 );
	}

	// setup vert structure to mirror layout in GPU
	struct VertexData
	{
		vec3 pos;
		vec3 n;
		vec2 tc;
	};

	const aiMesh* mesh = scene->mMeshes[0];
	std::vector<VertexData> vertices;
	for ( unsigned i = 0; i != mesh->mNumVertices; i++ )
	{
		const aiVector3D v = mesh->mVertices[i];
		const aiVector3D n = mesh->mNormals[i];
		const aiVector3D t = mesh->mTextureCoords[0][i];
		vertices.push_back ( {
			.pos = vec3 ( v.x, v.z, v.y ),
			.n = vec3 ( n.x, n.y, n.z ),
			.tc = vec2 ( t.x, t.y )
			} );
	}

	std::vector<unsigned int> indices;
	for ( unsigned i = 0; i != mesh->mNumFaces; i++ )
	{
		for ( unsigned j = 0; j != 3; j++ )
		{
			indices.push_back ( mesh->mFaces[i].mIndices[j] );
		}
	}

	aiReleaseImport ( scene );

	const size_t kSizeIndices = sizeof ( unsigned int ) * indices.size ();
	const size_t kSizeVertices = sizeof ( VertexData ) * vertices.size ();

	// indices
	GLuint dataIndices;
	glCreateBuffers ( 1, &dataIndices );
	glNamedBufferStorage ( dataIndices, kSizeIndices, indices.data (), 0 );
	GLuint vao;
	glCreateVertexArrays ( 1, &vao );
	glBindVertexArray ( vao );
	glVertexArrayElementBuffer ( vao, dataIndices );

	// vertices
	GLuint dataVertices;
	glCreateBuffers ( 1, &dataVertices );
	glNamedBufferStorage ( dataVertices, kSizeVertices, vertices.data (), 0 );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, 1, dataVertices );

	// model matrices
	GLuint modelMatrices;
	glCreateBuffers ( 1, &modelMatrices );
	glNamedBufferStorage ( modelMatrices, sizeof ( mat4 ) * 2, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, 2, modelMatrices );

	glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );

	// texture
	GLuint texture;
	{
		std::string textureFilename = ROOT_DIR + std::string ( "assets/models/rubber_duck/textures/Duck_baseColor.png" );
		int w, h, comp;
		const uint8_t* img = stbi_load ( textureFilename.c_str (), &w, &h, &comp, 3 );

		glCreateTextures ( GL_TEXTURE_2D, 1, &texture );
		glTextureParameteri ( texture, GL_TEXTURE_MAX_LEVEL, 0 );
		glTextureParameteri ( texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTextureParameteri ( texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTextureStorage2D ( texture, 1, GL_RGB8, w, h );
		glTextureSubImage2D ( texture, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img );
		glBindTextures ( 0, 1, &texture );

		stbi_image_free ( (void*)img );
	}

	// cube map
	GLuint cubemapTex;
	{
		std::string imgFilename = ROOT_DIR + std::string ( "assets/images/piazza_bologni_1k.hdr" );
		int w, h, comp;
		const float* img = stbi_loadf ( imgFilename.c_str (), &w, &h, &comp, 3 );
		Bitmap in ( w, h, comp, eBitmapFormat_Float, img );
		Bitmap out = convertEquirectangularMapToVerticalCross ( in );
		stbi_image_free ( (void*)img );

		Bitmap cubemap = convertVerticalCrossToCubeMapFaces ( out );

		glCreateTextures ( GL_TEXTURE_CUBE_MAP, 1, &cubemapTex );
		glTextureParameteri ( cubemapTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTextureParameteri ( cubemapTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTextureParameteri ( cubemapTex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
		glTextureParameteri ( cubemapTex, GL_TEXTURE_BASE_LEVEL, 0 );
		glTextureParameteri ( cubemapTex, GL_TEXTURE_MAX_LEVEL, 0 );
		glTextureParameteri ( cubemapTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTextureParameteri ( cubemapTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTextureStorage2D ( cubemapTex, 1, GL_RGB32F, cubemap.w_, cubemap.h_ );
		const uint8_t* data = cubemap.data_.data ();

		for ( unsigned i = 0; i != 6; ++i )
		{
			glTextureSubImage3D ( cubemapTex, 0, 0, 0, i, cubemap.w_, cubemap.h_, 1, GL_RGB, GL_FLOAT, data );
			data += cubemap.w_ * cubemap.h_ * cubemap.comp_ * Bitmap::getBytesPerComponent ( cubemap.fmt_ );
		}
		glBindTextures ( 1, 1, &cubemapTex );
	}

	double timeStamp = glfwGetTime ();
	float deltaSeconds = 0.0f;

	FramesPerSecondCounter fpsCounter ( 0.5f );


	// main loop
	while ( !glfwWindowShouldClose ( window ) )
	{
		fpsCounter.tick ( deltaSeconds );

		positioner.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft );

		const double newTimeStamp = glfwGetTime ();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		int width, height;
		glfwGetFramebufferSize ( window, &width, &height );
		const float ratio = width / (float)height;

		glViewport ( 0, 0, width, height );
		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		const mat4 p = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );
		const mat4 view = camera.getViewMatrix ();

		const PerFrameData perFrameData = {
			.view = view,
			.proj = p,
			.cameraPos = glm::vec4 ( camera.getPosition (), 1.0f )
		};

		glNamedBufferSubData ( perFrameDataBuffer, 0, kUniformBufferSize, &perFrameData );

		const mat4 Matrices[2]{
			glm::rotate ( glm::translate ( mat4 ( 1.0f ), vec3 ( 0.0f, -0.5f, -1.5f ) ), (float)glfwGetTime (), vec3 ( 0.0f, 1.0f, 0.0f ) ),
			glm::scale ( mat4 ( 1.0f ), vec3 ( 10.0f ) )
		};

		glNamedBufferSubData ( modelMatrices, 0, sizeof ( mat4 ) * 2, &Matrices );

		progModel.useProgram ();
		glDrawElementsInstancedBaseVertexBaseInstance ( GL_TRIANGLES, static_cast<unsigned>(indices.size ()), GL_UNSIGNED_INT, nullptr, 1, 0, 0 );

		progCube.useProgram ();
		glDrawArraysInstancedBaseInstance ( GL_TRIANGLES, 0, 36, 1, 1 );

		glfwSwapBuffers ( window );
		glfwPollEvents ();

		assert ( glGetError () == GL_NO_ERROR );
	}

	// Teardown
	glDeleteBuffers ( 1, &dataIndices );
	glDeleteBuffers ( 1, &dataVertices );
	glDeleteBuffers ( 1, &perFrameDataBuffer );
	glDeleteBuffers ( 1, &modelMatrices );
	glDeleteVertexArrays ( 1, &vao );

	glfwDestroyWindow ( window );
	glfwTerminate ();

	exit ( EXIT_SUCCESS );
}