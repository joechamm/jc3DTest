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

#include "ResourceString.h"

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
	glfwSwapInterval ( 1 );

	std::string meshFilename = appendToRoot ( "assets/models/Exterior/test.meshes" );

	FILE* f = fopen ( meshFilename.c_str (), "rb" );
	if ( !f )
	{
		printf ( "Unable to open mesh file %s\n", meshFilename.c_str () );
		exit ( 255 );
	}

	MeshFileHeader header;
	if ( fread ( &header, 1, sizeof ( header ), f ) != sizeof ( header ) )
	{
		printf ( "Unable to read mesh file header %s\n", meshFilename.c_str () );
		exit ( 255 );
	}

	std::vector<Mesh> meshes1;
	const auto meshCount = header.meshCount;
	meshes1.resize ( meshCount );
	if ( fread ( meshes1.data (), sizeof ( Mesh ), meshCount, f ) != meshCount )
	{
		printf ( "Could not read meshes\n" );
		exit ( 255 );
	}

	std::vector<uint32_t> indexData;
	std::vector<float> vertexData;
	const auto idxDataSize = header.indexDataSize;
	const auto vtxDataSize = header.vertexDataSize;
	indexData.resize ( idxDataSize / sizeof ( uint32_t ) );
	vertexData.resize ( vtxDataSize / sizeof ( float ) );
	if ( (fread ( indexData.data (), 1, idxDataSize, f ) != idxDataSize) ||
		(fread ( vertexData.data (), 1, vtxDataSize, f ) != vtxDataSize) )
	{
		printf ( "Unable to read index/vertex data" );
		exit ( 255 );
	}

	fclose ( f );

	GLMesh mesh ( indexData.data (), idxDataSize, vertexData.data (), vtxDataSize );

	// load shaders
	std::string shdGridVtxFilename = appendToRoot ( "assets/shaders/GL05_grid.vert" );
	std::string shdGridFragFilename = appendToRoot ( "assets/shaders/GL05_grid.frag" );

	GLShader shdGridVertex ( shdGridVtxFilename.c_str () );
	GLShader shdGridFragment ( shdGridFragFilename.c_str () );
	GLProgram progGrid ( shdGridVertex, shdGridFragment );
	progGrid.useProgram ();

	std::string shdMeshVtxFilename = appendToRoot ( "assets/shaders/GL05_mesh_inst.vert" );
	std::string shdMeshGeomFilename = appendToRoot ( "assets/shaders/GL05_mesh_inst.geom" );
	std::string shdMeshFragFilename = appendToRoot ( "assets/shaders/GL05_mesh_inst.frag" );

	GLShader shaderMeshVertex ( shdMeshVtxFilename.c_str () );
	GLShader shaderMeshGeom ( shdMeshGeomFilename.c_str () );
	GLShader shaderMeshFrag ( shdMeshFragFilename.c_str () );
	GLProgram progMesh ( shaderMeshVertex, shaderMeshGeom, shaderMeshFrag );

	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );

	GLBuffer perFrameDataBuffer ( kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle (), 0, kUniformBufferSize );

	const mat4 m ( 1.0f );
	GLBuffer modelMatrices ( sizeof ( mat4 ), value_ptr ( m ), GL_DYNAMIC_STORAGE_BIT );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, 2, modelMatrices.getHandle () );

	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	glEnable ( GL_BLEND );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable ( GL_DEPTH_TEST );

	double timeStamp = glfwGetTime ();
	float deltaSeconds = 0.0f;

	// main loop
	while ( !glfwWindowShouldClose ( window ) )
	{
		positioner.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft );

		const double newTimeStamp = glfwGetTime ();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		int width, height;
		glfwGetFramebufferSize ( window, &width, &height );
		const float ratio = width / (float)height;

		glViewport ( 0, 0, width, height );
		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		const mat4 p = glm::perspective ( 45.0f, ratio, 0.5f, 5000.0f );
		const mat4 view = camera.getViewMatrix ();

		const PerFrameData perFrameData = {
			.view = view,
			.proj = p,
			.cameraPos = glm::vec4 ( camera.getPosition (), 1.0f )
		};

		glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, kUniformBufferSize, &perFrameData );

		glDisable ( GL_DEPTH_TEST );
		glEnable ( GL_BLEND );
		progGrid.useProgram ();
		glDrawArraysInstancedBaseInstance ( GL_TRIANGLES, 0, 6, 1, 0 );

		glEnable ( GL_DEPTH_TEST );
		glDisable ( GL_BLEND );
		progMesh.useProgram ();
		mesh.draw ();		

		glfwSwapBuffers ( window );
		glfwPollEvents ();
	}

	// Teardown
	
	glfwDestroyWindow ( window );
	glfwTerminate ();

	exit ( EXIT_SUCCESS );
}