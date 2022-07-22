#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <glfw/glfw3.h>
#include "glFramework/GLShader.h"
#include "UtilsMath.h"
#include "Camera.h"
#include "ResourceString.h"
#include "glFramework/UtilsGLImGui.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;
using glm::ivec2;
using std::string;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
	float tessellationScale;
};

struct MouseState
{
	glm::vec2 pos = glm::vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

float tessellationScale = 1.0f;

CameraPositioner_FirstPerson positioner ( vec3 ( 0.0f, 0.5f, 0.0f ), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
Camera camera ( positioner );

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

	GLFWwindow* window = glfwCreateWindow ( 1280, 720, "3D Graphics Rendering Cookbook Chapter 05 Tessellation Example", nullptr, nullptr );
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

	glfwSetCursorPosCallback (
		window,
		[]( auto* window, double x, double y )
		{
			int width, height;
			glfwGetFramebufferSize ( window, &width, &height );
			mouseState.pos.x = static_cast<float>(x / width);
			mouseState.pos.y = static_cast<float>(y / height);

			ImGui::GetIO ().MousePos = ImVec2 ( (float)x, (float)y );
		}
	);

	glfwSetMouseButtonCallback (
		window,
		[]( auto* window, int button, int action, int mods )
		{
			if ( button == GLFW_MOUSE_BUTTON_LEFT )
				mouseState.pressedLeft = action == GLFW_PRESS;

			auto& io = ImGui::GetIO ();
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			io.MouseDown[idx] = action == GLFW_PRESS;
		}
	);

	glfwMakeContextCurrent ( window );
	gladLoadGL ( glfwGetProcAddress );
	glfwSwapInterval ( 1 );
		
	// asset filenames
	
	// grid shader
	string shdGridVtFname = appendToRoot ( "assets/shaders/GL05_grid.vert" );
	string shdGridFrFname = appendToRoot ( "assets/shaders/GL05_grid.frag" );
	// duck shader
	string shdDuckVtFname = appendToRoot ( "assets/shaders/GL05_duck.vert" );
	string shdDuckTcFname = appendToRoot ( "assets/shaders/GL05_duck.tesc" );
	string shdDuckTeFname = appendToRoot ( "assets/shaders/GL05_duck.tese" );
	string shdDuckGeFname = appendToRoot ( "assets/shaders/GL05_duck.geom" );
	string shdDuckFrFname = appendToRoot ( "assets/shaders/GL05_duck.frag" );
	// duck scene
	string sceneDuckFname = appendToRoot ( "assets/models/rubber_duck/scene.gltf" );
	// duck texture
	string textureDuckFname = appendToRoot ( "assets/models/rubber_duck/textures/Duck_baseColor.png" );

	// grid shader program
	GLShader shdGridVertex ( shdGridVtFname.c_str () );
	GLShader shdGridFragment ( shdGridFrFname.c_str () );
	GLProgram progGrid ( shdGridVertex, shdGridFragment );

	// duck shader program
	GLShader shdDuckVertex ( shdDuckVtFname.c_str () );
	GLShader shdDuckTessControl ( shdDuckTcFname.c_str () );
	GLShader shdDuckTessEval ( shdDuckTeFname.c_str () );
	GLShader shdDuckGeom ( shdDuckGeFname.c_str () );
	GLShader shdDuckFrag ( shdDuckFrFname.c_str () );
	GLProgram progDuck ( shdDuckVertex, shdDuckTessControl, shdDuckTessEval, shdDuckGeom, shdDuckFrag );

	// SETUP uniform buffer 
	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );

	GLBuffer perFrameDataBuffer ( kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle (), 0, kUniformBufferSize );

	GLuint vao;
	glCreateVertexArrays ( 1, &vao );
	glBindVertexArray ( vao );

	// Set the clear color and enable depth testing
	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	glEnable ( GL_BLEND );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable ( GL_DEPTH_TEST );

	const aiScene* scene = aiImportFile ( sceneDuckFname.c_str (), aiProcess_Triangulate );

	// Error checking
	if ( !scene || !scene->HasMeshes () )
	{
		printf ( "Unable to load %s\n", sceneDuckFname.c_str () );
		exit ( 255 );
	}

	// setup vert structure to mirror layout in GPU
	struct VertexData
	{
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
	glVertexArrayElementBuffer ( vao, dataIndices );

	// vertices
	GLuint dataVertices;
	glCreateBuffers ( 1, &dataVertices );
	glNamedBufferStorage ( dataVertices, kSizeVertices, vertices.data (), 0 );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, 1, dataVertices );

	// model matrices
	GLuint modelMatrices;
	glCreateBuffers ( 1, &modelMatrices );
	glNamedBufferStorage ( modelMatrices, sizeof ( mat4 ), nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, 2, modelMatrices );

	// texture
	int w, h, comp;
	const uint8_t* img = stbi_load ( textureDuckFname.c_str (), &w, &h, &comp, 3 );

	GLuint texture;
	glCreateTextures ( GL_TEXTURE_2D, 1, &texture );
	glTextureParameteri ( texture, GL_TEXTURE_MAX_LEVEL, 0 );
	glTextureParameteri ( texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri ( texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTextureStorage2D ( texture, 1, GL_RGB8, w, h );
	glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
	glTextureSubImage2D ( texture, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img );

	stbi_image_free ( (void*)img );

	double timeStamp = glfwGetTime ();
	float deltaSeconds = 0.0f;

	ImGuiGLRenderer rendererUI;

	// main loop
	while ( !glfwWindowShouldClose ( window ) )
	{
		ImGuiIO& io = ImGui::GetIO ();

		positioner.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft && !io.WantCaptureMouse );
		
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
			.cameraPos = glm::vec4 ( camera.getPosition (), 1.0f ),
			.tessellationScale = tessellationScale
		};

		glNamedBufferSubData ( perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData );

		const mat4 s = glm::scale ( mat4 ( 1.0f ), vec3 ( 10.0f ) );
		const mat4 m = s * glm::rotate ( glm::translate ( mat4 ( 1.0f ), vec3 ( 0.0f, -0.5f, -1.5f ) ), (float)glfwGetTime () * 0.1f, vec3 ( 0.0f, 1.0f, 0.0f ) );
		glNamedBufferSubData ( modelMatrices, 0, sizeof ( mat4 ), glm::value_ptr ( m ) );

		glBindVertexArray ( vao );
		glEnable ( GL_DEPTH_TEST );

		glDisable ( GL_BLEND );
		progDuck.useProgram ();
		glBindTextures ( 0, 1, &texture );
		glDrawElements ( GL_PATCHES, static_cast<unsigned>(indices.size ()), GL_UNSIGNED_INT, nullptr );

		glEnable ( GL_BLEND );
		progGrid.useProgram ();
		glDrawArraysInstancedBaseInstance ( GL_TRIANGLES, 0, 6, 1, 0 );

		io.DisplaySize = ImVec2 ( (float)width, (float)height );
		ImGui::NewFrame ();
		ImGui::SliderFloat ( "Tessellation scale", &tessellationScale, 1.0f, 2.0f, "%.1f" );
		ImGui::Render ();
		rendererUI.render ( width, height, ImGui::GetDrawData () );

		glfwSwapBuffers ( window );
		glfwPollEvents ();

		assert ( glGetError () == GL_NO_ERROR );
	}

	// Teardown
	glDeleteTextures ( 1, &texture );

	glDeleteBuffers ( 1, &dataIndices );
	glDeleteBuffers ( 1, &dataVertices );
	glDeleteBuffers ( 1, &modelMatrices );
	glDeleteVertexArrays ( 1, &vao );

	glfwDestroyWindow ( window );
	glfwTerminate ();

	exit ( EXIT_SUCCESS );
}