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
#include "debug.h"

#include <vector>

#include <helpers/RootDir.h>

using glm::mat4;
using glm::vec3;
using glm::vec2;

struct PerFrameData {
	mat4 mvp;
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

	GLFWwindow* window = glfwCreateWindow ( 1280, 720, "Programmable Vertex Pulling example", nullptr, nullptr );
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
			if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
		}
	);


	glfwMakeContextCurrent ( window );
	gladLoadGL ();
	glfwSwapInterval ( 1 );

	// setup debugging
	initDebug ();

	// load shaders
	std::string vertShaderFilename = ROOT_DIR + std::string ( "assets/shaders/GL02.vert" );
	std::string geomShaderFilename = ROOT_DIR + std::string ( "assets/shaders/GL02.geom" );
	std::string fragShaderFilename = ROOT_DIR + std::string ( "assets/shaders/GL02.frag" );
	GLShader shaderVertex ( vertShaderFilename.c_str() );
	GLShader shaderGeometry ( geomShaderFilename.c_str() );
	GLShader shaderFragment ( fragShaderFilename.c_str() );
	GLProgram program ( shaderVertex, shaderGeometry, shaderFragment );
	program.useProgram ();

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
	const aiScene* scene = aiImportFile ( sceneFilename.c_str(), aiProcess_Triangulate );

	// Error checking
	if ( !scene || !scene->HasMeshes () )
	{
		printf ( "Unable to load assets/models/rubber_duck/scene.gltf\n" );
		exit ( 255 );
	}

	// setup vert structure to mirror layout in GPU
	struct VertexData {
		vec3 pos;
		vec2 tc;
	};

	const aiMesh* mesh = scene->mMeshes[0];
	std::vector<VertexData> vertices;
	for ( unsigned i = 0; i != mesh->mNumVertices; i++ )
	{
		const aiVector3D v = mesh->mVertices[i];
		const aiVector3D t = mesh->mTextureCoords[0][i];		
		vertices.push_back ( {
			.pos = vec3 ( v.x, v.z, v.y ),
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

	std::string textureFilename = ROOT_DIR + std::string ( "assets/models/rubber_duck/textures/Duck_baseColor.png" );
	// texture
	int w, h, comp;
	const uint8_t* img = stbi_load ( textureFilename.c_str(), &w, &h, &comp, 3 );

	GLuint texture;
	glCreateTextures ( GL_TEXTURE_2D, 1, &texture );
	glTextureParameteri ( texture, GL_TEXTURE_MAX_LEVEL, 0 );
	glTextureParameteri ( texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTextureParameteri ( texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTextureStorage2D ( texture, 1, GL_RGB8, w, h );
	glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
	glTextureSubImage2D ( texture, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img );
	glBindTextures ( 0, 1, &texture );

	stbi_image_free ( (void*)img );

	// main loop
	while ( !glfwWindowShouldClose ( window ) )
	{
		int width, height;
		glfwGetFramebufferSize ( window, &width, &height );
		const float ratio = width / (float)height;

		glViewport ( 0, 0, width, height );
		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		const mat4 m = glm::rotate ( glm::translate ( mat4 ( 1.0f ), vec3 ( 0.0f, -0.5f, -1.5f ) ), (float)glfwGetTime (), vec3 ( 0.0f, 1.0f, 0.0f ) );
		const mat4 p = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );

		PerFrameData perFrameData = { .mvp = p * m };
		glNamedBufferSubData ( perFrameDataBuffer, 0, kUniformBufferSize, &perFrameData );
		glDrawElements ( GL_TRIANGLES, static_cast<unsigned>( indices.size () ), GL_UNSIGNED_INT, nullptr );

		glfwSwapBuffers ( window );
		glfwPollEvents ();
	}

	// Teardown
	glDeleteBuffers ( 1, &dataIndices );
	glDeleteBuffers ( 1, &dataVertices );
	glDeleteBuffers ( 1, &perFrameDataBuffer );
	glDeleteVertexArrays ( 1, &vao );
	
	glfwDestroyWindow ( window );
	glfwTerminate ();

	exit ( EXIT_SUCCESS );
}