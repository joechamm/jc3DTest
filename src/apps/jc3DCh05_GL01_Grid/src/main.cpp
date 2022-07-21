#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "debug.h"
//#include "glFramework/GLFWApp.h"
#include "glFramework/GLShader.h"
#include "UtilsMath.h"
#include "debug.h"
#include "Camera.h"
#include "ResourceString.h"

#include <helpers/RootDir.h>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;
using glm::ivec2;

using std::string;
using std::cout;
using std::endl;

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

CameraPositioner_FirstPerson positioner ( vec3 ( 0.0f ), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
Camera camera ( positioner );

GLFWwindow* g_window;

const vec4 pos[4] = {
	vec4 ( -1.0f, 0.0f, -1.0f, 1.0f ),
	vec4 ( 1.0, 0.0, -1.0, 1.0f ),
	vec4 ( 1.0, 0.0, 1.0, 1.0f ),
	vec4 ( -1.0, 0.0, 1.0, 1.0f )
};

const uint32_t indices[6] = {
	0, 1, 2,
	2, 3, 0
};

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

#include <vector>
#include <iostream>

#include "helpers/RootDir.h"

using glm::mat4;
using glm::vec3;

static const char* shaderCodeVertex = R"(
#version 460 core

layout (location = 0) in vec4 aPos;

layout(std140, binding = 0) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

// extents of grid in world coordinates
float gridSize = 100.0;

// size of one cell
float gridCellSize = 0.025;

// color of thin lines
vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.0);

// color of thick lines (every tenth line)
vec4 gridColorThick = vec4(0.0, 0.0, 0.0, 1.0);

// minimum number of pixels between cell lines before LOD switch should occur.
const float gridMinPixelsBetweenCells = 2.0;

const vec3 pos[4] = vec3[4](
	vec3(-1.0, 0.0, -1.0),
	vec3( 1.0, 0.0, -1.0),
	vec3( 1.0, 0.0,  1.0),
	vec3(-1.0, 0.0,  1.0)
);

const int indices[6] = int[6](
	0, 1, 2, 2, 3, 0
);

layout (location = 0) out vec2 uv;

void main()
{
	mat4 MVP = proj * view;

//	int idx = indices[gl_VertexID];
//	vec3 position = pos[idx] * gridSize;
	vec3 position = aPos.xyz * gridSize;
	int idx = indices[gl_VertexID];

	gl_Position = MVP * vec4(position, 1.0);
	uv = position.xz;
}
)";

static const char* shaderCodeFragment = R"(
#version 460 core 

// extents of grid in world coordinates
float gridSize = 100.0;

// size of one cell
float gridCellSize = 0.025;

// color of thin lines
vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.0);

// color of thick lines (every tenth line)
vec4 gridColorThick = vec4(0.0, 0.0, 0.0, 1.0);

// minimum number of pixels between cell lines before LOD switch should occur. 
const float gridMinPixelsBetweenCells = 2.0;

float log10(float x)
{
	return log(x) / log(10.0);
}

float satf(float x)
{
	return clamp(x, 0.0, 1.0);
}

vec2 satv(vec2 x)
{
	return clamp(x, vec2(0.0), vec2(1.0));
}

float max2(vec2 v)
{
	return max(v.x, v.y);
}

vec4 gridColor(vec2 uv)
{
	// let uv = (F(s, t), G(s, t)) { function from R^2 -> R^2 }
	// dudv.s { first coordinate of dudv } = 
	//		sqrt((dF/ds)^2 + (dF/dt)^2)
	// dudv.t { second coordinate of dudv } = 
	//		sqrt((dG/ds)^2 + (dG/dt)^2)
	vec2 dudv = vec2(
		length(vec2(dFdx(uv.x), dFdy(uv.x))),
		length(vec2(dFdx(uv.y), dFdy(uv.y)))
	);

	float lodLevel = max(0.0, log10((length(dudv) * gridMinPixelsBetweenCells) / gridCellSize) + 1.0);
	float lodFade = fract(lodLevel);

	// cell sizes for lod0, lod1, and lod2 
	float lod0 = gridCellSize * pow(10.0, floor(lodLevel));
	float lod1 = lod0 * 10.0;
	float lod2 = lod1 * 10.0;

	// each anti-aliased line covers up to 4 pixels 
	dudv *= 4.0;

	// calculate absolute distances to cell line centers for each lod and pick max X/Y to get coverage alpha value 
	float lod0a = max2(vec2(1.0) - abs(satv(mod(uv, lod0) / dudv) * 2.0 - vec2(1.0)) );
	float lod1a = max2(vec2(1.0) - abs(satv(mod(uv, lod1) / dudv) * 2.0 - vec2(1.0)) );
	float lod2a = max2(vec2(1.0) - abs(satv(mod(uv, lod2) / dudv) * 2.0 - vec2(1.0)) );

	// blend between falloff colors to handle LOD transition 
	vec4 c = lod2a > 0.0 ? gridColorThick : lod1a > 0.0 ? mix(gridColorThick, gridColorThin, lodFade) : gridColorThin;

	// calculate the opacity falloff based on distance to grid extents 
	float opacityFalloff = (1.0 - satf(length(uv) / gridSize));

	// blend between LOD level alphas and scale with opacity falloff 
	c.a *= (lod2a > 0.0 ? lod2a : lod1a > 0.0 ? lod1a : (lod0a * (1.0 - lodFade))) * opacityFalloff;

	return c;
}

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 out_FragColor;

void main()
{
	out_FragColor = gridColor(uv);
};
)";

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

	GLFWwindow* window = glfwCreateWindow ( 1280, 720, "Chapter 05 Grid Example", nullptr, nullptr );
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
	gladLoadGL ( glfwGetProcAddress );
	glfwSwapInterval ( 1 );

#ifdef _DEBUG
	int success;
	char infoLog[512];
#endif // _DEBUG

	const GLuint shaderVertex = glCreateShader ( GL_VERTEX_SHADER );
	glShaderSource ( shaderVertex, 1, &shaderCodeVertex, nullptr );
	glCompileShader ( shaderVertex );

#ifdef _DEBUG
	glGetShaderiv ( shaderVertex, GL_COMPILE_STATUS, &success );

	if ( !success )
	{
		glGetShaderInfoLog ( shaderVertex, 512, nullptr, infoLog );
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	else
	{
		std::cout << "SUCCESS::SHADER::VERTEX::COMPILATION_SUCCESS!" << std::endl;
	}
#endif // _DEBUG

	const GLuint shaderFragment = glCreateShader ( GL_FRAGMENT_SHADER );
	glShaderSource ( shaderFragment, 1, &shaderCodeFragment, nullptr );
	glCompileShader ( shaderFragment );

#ifdef _DEBUG
	glGetShaderiv ( shaderFragment, GL_COMPILE_STATUS, &success );

	if ( !success )
	{
		glGetShaderInfoLog ( shaderFragment, 512, nullptr, infoLog );
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	else
	{
		std::cout << "SUCCESS::SHADER::FRAGMENT::COMPILATION_SUCCESS!" << std::endl;
	}
#endif // _DEBUG

	const GLuint program = glCreateProgram ();
	glAttachShader ( program, shaderVertex );
	glAttachShader ( program, shaderFragment );
	glLinkProgram ( program );

#ifdef _DEBUG
	glGetProgramiv ( program, GL_LINK_STATUS, &success );
	if ( !success )
	{
		glGetProgramInfoLog ( program, 512, nullptr, infoLog );
		std::cout << "ERROR:SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	else
	{
		std::cout << "SUCCESS::SHADER::PROGRAM::LINKING_SUCCESS!" << std::endl;
	}
#endif // _DEBUG

	const GLsizeiptr kBufferSize = sizeof ( PerFrameData );
	// SETUP uniform buffer 
	GLuint perFrameDataBuffer;
	glCreateBuffers ( 1, &perFrameDataBuffer );
	glNamedBufferStorage ( perFrameDataBuffer, kBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kBufferSize );
	// Set the clear color and enable depth testing
	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	glEnable ( GL_DEPTH_TEST );
	// enable polygon offset line for drawing wireframe
	glEnable ( GL_POLYGON_OFFSET_LINE );
	glPolygonOffset ( -1.0f, -1.0f );


	const vec4 pos[4] = {
		vec4 ( -1.0, 0.0, -1.0, 1.0f ),
		vec4 ( 1.0, 0.0, -1.0, 1.0f ),
		vec4 ( 1.0, 0.0, 1.0, 1.0f ),
		vec4 ( -1.0, 0.0, 1.0, 1.0f )
	};

	const uint32_t indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	// SETUP vertex array object
	GLuint vao;
	glCreateVertexArrays ( 1, &vao );
	glBindVertexArray ( vao );

	// SETUP mesh data vertex buffer and upload position to it
	GLuint meshData;
	glCreateBuffers ( 1, &meshData );
	glNamedBufferStorage ( meshData, sizeof ( 4 ) * 4, glm::value_ptr(pos[0]), 0 );

	GLuint indexBuffer;
	glCreateBuffers ( 1, &indexBuffer );
	glNamedBufferStorage ( indexBuffer, sizeof ( uint32_t ) * 6, &indices[0], 0 );

	// associate the meshData buffer with our vertex array
	glVertexArrayVertexBuffer ( vao, 0, meshData, 0, sizeof ( 4 ) );		/// @signature glVertexArrayVertexBuffer (GLuint vertexArrayObject, GLuint bindingIndex, GLuint buffer, GLintptr offset, GLsizei stride );	
	glEnableVertexArrayAttrib ( vao, 0 );									/// @signature glEnableVertexArrayAttrib (GLuint vertexArrayObject, GLuint bindingIndex)
	glVertexArrayAttribFormat ( vao, 0, 4, GL_FLOAT, GL_FALSE, 0 );			/// @signature glVertexArrayAttribFormat (GLuint vertexArrayObject, GLuint attribIndex, GLint size, GLenum type, GLboolean normalized, GLuint relativeOffset)
	glVertexArrayAttribBinding ( vao, 0, 0 );								/// @signature glVertexArrayAttribBinding (GLuint vertexArrayObject, GLuint attribIndex, Guint bindingIndex)

	glVertexArrayElementBuffer ( vao, indexBuffer );

	const int numVertices = 6;

	// main loop
	while ( !glfwWindowShouldClose ( window ) )
	{
		int width, height;
		glfwGetFramebufferSize ( window, &width, &height );
		const float ratio = width / (float)height;

		glViewport ( 0, 0, width, height );
		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		const mat4 m = glm::rotate ( glm::translate ( mat4 ( 1.0f ), vec3 ( 0.0f, -0.5f, -1.5f ) ), (float)glfwGetTime (), vec3 ( 0.0f, 1.0f, 0.0f ) );
		const mat4 projMat = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );

		const mat4 viewMat = camera.getViewMatrix ();
		const vec3 camPos = camera.getPosition ();

		PerFrameData perFrameData = {
			.view = viewMat,
			.proj = projMat,
			.cameraPos = vec4(camPos, 1.0f)
		};

		glUseProgram ( program );
		glNamedBufferSubData ( perFrameDataBuffer, 0, kBufferSize, &perFrameData );

		glPolygonMode ( GL_FRONT_AND_BACK, GL_FILL );
		glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr );
//		glDrawArrays ( GL_TRIANGLES, 0, numVertices );

		glNamedBufferSubData ( perFrameDataBuffer, 0, kBufferSize, &perFrameData );

		glPolygonMode ( GL_FRONT_AND_BACK, GL_LINE );
		glDrawArrays ( GL_TRIANGLES, 0, numVertices );

		glfwSwapBuffers ( window );
		glfwPollEvents ();
	}

	// Teardown
	glDeleteBuffers ( 1, &meshData );
	glDeleteBuffers ( 1, &perFrameDataBuffer );
	glDeleteProgram ( program );
	glDeleteShader ( shaderFragment );
	glDeleteShader ( shaderVertex );
	glDeleteVertexArrays ( 1, &vao );

	glfwDestroyWindow ( window );
	glfwTerminate ();

	exit ( EXIT_SUCCESS );
}

/*
int main ( void )
{
	glfwSetErrorCallback (
		[]( int err, const char* description ) {
			fprintf ( stderr, "Error: %s\n", description );
		}
	);

	if ( !glfwInit () )
	{
		exit ( EXIT_FAILURE );
	}

	glfwWindowHint ( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint ( GLFW_CONTEXT_VERSION_MINOR, 6 );
	glfwWindowHint ( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
//	glfwWindowHint ( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );

	g_window = glfwCreateWindow ( 1280, 720, "jc3DCh05_GL01_Grid Exercise", nullptr, nullptr );

	if ( !g_window )
	{
		glfwTerminate ();
		exit ( EXIT_FAILURE );
	}

	glfwSetCursorPosCallback (
		g_window,
		[]( auto* window, double x, double y ) {
			int width, height;
			glfwGetFramebufferSize ( window, &width, &height );
			mouseState.pos.x = static_cast<float>(x / width);
			mouseState.pos.y = static_cast<float>(y / height);
		}
	);

	glfwSetMouseButtonCallback (
		g_window,
		[]( auto* window, int button, int action, int mods ) {
			if ( button == GLFW_MOUSE_BUTTON_LEFT )
			{
				mouseState.pressedLeft = action == GLFW_PRESS;
			}
		}
	);

	glfwSetKeyCallback (
		g_window,
		[]( GLFWwindow* window, int key, int scancode, int action, int mods ) {
			const bool pressed = action != GLFW_RELEASE;
			if ( key == GLFW_KEY_ESCAPE && pressed )
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
			if ( key == GLFW_KEY_W )
				positioner.movement_.forward_ = pressed;
			if ( key == GLFW_KEY_S )
				positioner.movement_.backward_ = pressed;
			if ( key == GLFW_KEY_A )
				positioner.movement_.left_ = pressed;
			if ( key == GLFW_KEY_D )
				positioner.movement_.right_ = pressed;
			if ( key == GLFW_KEY_1 )
				positioner.movement_.up_ = pressed;
			if ( key == GLFW_KEY_2 )
				positioner.movement_.down_ = pressed;
			if ( mods & GLFW_MOD_SHIFT )
				positioner.movement_.fastSpeed_ = pressed;
			if ( key == GLFW_KEY_SPACE )
				positioner.setUpVector ( vec3 ( 0.0f, 1.0f, 0.0f ) );
		}
	);

	glfwMakeContextCurrent ( g_window );
	gladLoadGL ( glfwGetProcAddress );
	glfwSwapInterval ( 1 );

//	initDebug ();

	string vtxShaderFilename = appendToRoot ( "assets/shaders/GL05_grid.vert" );
	string fragShaderFilename = appendToRoot ( "assets/shaders/GL05_grid.frag" );

	GLShader vtxShdGrid ( vtxShaderFilename.c_str () );

	cout << "vertex shader " << vtxShaderFilename << " handle: " << vtxShdGrid.getHandle () << endl;

	GLShader fragShdGrid ( fragShaderFilename.c_str () );

	cout << "fragment shader " << fragShaderFilename << " handle: " << fragShdGrid.getHandle () << endl;

	GLProgram progGrid ( vtxShdGrid, fragShdGrid );

	cout << "progGrid handle: " << progGrid.getHandle () << endl;

	const PerFrameData perFrameInit = { 
		.view = camera.getViewMatrix(),
		.proj = glm::perspective(45.0f, static_cast<float>(1280.0f / 720.0f), 0.1f, 1000.0f), 
		.cameraPos = glm::vec4 ( camera.getPosition (), 1.0f ) 
	};

	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );

	GLuint perFrameDataBuffer;
	glCreateBuffers ( 1, &perFrameDataBuffer );
	glNamedBufferStorage ( perFrameDataBuffer, kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kUniformBufferSize );
	glNamedBufferSubData ( perFrameDataBuffer, 0, kUniformBufferSize, &perFrameInit );

	const GLsizeiptr kVertexBufferSize = 4 * sizeof(vec4);
	
	GLuint vertexBuffer;
	glCreateBuffers ( 1, &vertexBuffer );
	glNamedBufferStorage ( vertexBuffer, kVertexBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
//	glBindBufferRange ( GL_ARRAY_BUFFER, 0, vertexBuffer, 0, kVertexBufferSize );
	glNamedBufferSubData ( vertexBuffer, 0, kVertexBufferSize, glm::value_ptr(pos[0]) );

	const GLsizeiptr kIndexBufferSize = 6 * sizeof ( uint32_t );

	GLuint indexBuffer;
	glCreateBuffers ( 1, &indexBuffer );
	glNamedBufferStorage ( indexBuffer, kIndexBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
//	glBindBufferRange ( GL_ELEMENT_ARRAY_BUFFER, 0, indexBuffer, 0, kIndexBufferSize );
	glNamedBufferSubData ( indexBuffer, 0, kIndexBufferSize, &indices[0] );

	GLuint vao;
	glCreateVertexArrays ( 1, &vao );
	glVertexArrayElementBuffer ( vao, indexBuffer );
	glVertexArrayVertexBuffer ( vao, 0, vertexBuffer, 0, sizeof ( vec4 ) );
	glEnableVertexArrayAttrib ( vao, 0 );
	glVertexArrayAttribFormat ( vao, 0, 4, GL_FLOAT, GL_FALSE, 0 );
	glVertexArrayAttribBinding ( vao, 0, 0 );
	
	glBindVertexArray ( vao );

	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	glEnable ( GL_BLEND );
	glEnable ( GL_DEPTH_TEST );
	glBlendEquation ( GL_FUNC_ADD );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable ( GL_CULL_FACE );
	glEnable ( GL_SCISSOR_TEST );

	double timeStamp = glfwGetTime ();
	float deltaSeconds = 0.0f;

	while ( !glfwWindowShouldClose ( g_window ) )
	{
		positioner.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft );

		const double newTimeStamp = glfwGetTime ();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		std::cout << "time: " << timeStamp << std::endl;

		int width, height;
		glfwGetFramebufferSize ( g_window, &width, &height );
		const float ratio = width / (float)height;

		glViewport ( 0, 0, width, height );
		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		const mat4 p = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );
		const mat4 view = camera.getViewMatrix ();

		const PerFrameData perFrameData = { .view = view, .proj = p, .cameraPos = glm::vec4 ( camera.getPosition (), 1.0f ) };
		glNamedBufferSubData ( perFrameDataBuffer, 0, kUniformBufferSize, &perFrameData );

		progGrid.useProgram ();
		glBindVertexArray ( vao );
		glDrawElementsBaseVertex ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, 0 );
//		glDrawArraysInstancedBaseInstance ( GL_TRIANGLES, 0, 6, 1, 0 );

		glScissor ( 0, 0, width, height );

		glfwSwapBuffers ( g_window );
		glfwPollEvents ();

		GLenum err = glGetError ();
		if ( err != GL_NO_ERROR )
		{
			std::cout << "glGetError returned: " << err << std::endl;
		}
	}

	glDeleteBuffers ( 1, &perFrameDataBuffer );
	glDeleteBuffers ( 1, &vertexBuffer );
	glDeleteBuffers ( 1, &indexBuffer );
	glDeleteVertexArrays ( 1, &vao );

	return 0;
}
*/