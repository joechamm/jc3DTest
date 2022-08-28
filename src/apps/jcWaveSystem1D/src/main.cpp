#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
//#include <jcGLframework/debug.h>
#include <jcCommonFramework/ResourceString.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "WaveSystem1D.h"

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;

constexpr GLuint kIdxBind_UpdateRead = 0;
constexpr GLuint kIdxBind_UpdateWrite = 1;
constexpr GLuint kIdxBind_WaveVerticesIn = 1;
constexpr GLint kIdxUniLoc_uProjectionMatrix = 0;
constexpr GLint kIdxUniLoc_uViewMatrix = 1;
constexpr GLint kIdxUniLoc_uModelMatrix = 2;
constexpr GLint kIdxuniLoc_uBoxPercentage = 3;

constexpr vec3 kLookAtVec = vec3 ( 0.0f, 0.0f, 0.0f );
constexpr vec3 kEyePosVec = vec3 ( 0.0f, 0.0f, -2.0f );
constexpr vec3 kUpVec = vec3 ( 0.0f, 1.0f, 0.0f );

enum class eProjectMode : uint8_t
{
	eProjectMode_None = 0x00,
	eProjectMode_Ortho = 0x01,
	eProjectMode_Persp = 0x02,
	eProjectMode_Count = 0x03
} eProjectMode_current;

enum class eViewMode : uint8_t
{
	eViewMode_None = 0x00,
	eViewMode_LookAt = 0x01,
	eViewMode_Count = 0x02
} eViewMode_current;

bool g_paused = true;
mat4 g_proj = mat4 ( 1.0f );
mat4 g_view = mat4 ( 1.0f );
mat4 g_model = mat4 ( 1.0f );
float g_box_percentage = 0.9f;
uint32_t g_num_points = 256;
float g_dt = 0.1f;
float g_wave_speed = 1.0f;

WaveSystem1D* g_pWaveSystem = nullptr;

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

void initVerticesTriangleWave ( std::vector<Wave1DVert>& initialState, uint32_t vertCount )
{
	constexpr float twopi = glm::two_pi<float> ();
	float dx = 1.0f / ( static_cast< float >( vertCount ) - 1.0f );
	float x = 0.0f;
	for ( uint32_t i = 0; i < vertCount; ++i )
	{
		if ( x < 0.25f )
		{
			initialState [ i ].height = 4.0f * x;
		}
		else if ( x < 0.75f )
		{
			initialState [ i ].height = -4.0f * x + 2.0f;
		}
		else
		{
			initialState [ i ].height = 4.0f * x - 4.0f;
		}

		initialState [ i ].velocity = cosf ( twopi * x );
		x += dx;
	}
}

int main ( int argc, char** argv )
{
	GLFWApp app;

	eProjectMode_current = eProjectMode::eProjectMode_Ortho;
	eViewMode_current = eViewMode::eViewMode_None;

	GLShader shdRenderVert ( appendToRoot ( "assets/shaders/wave_system1D_render.vert" ).c_str () );
	GLShader shdRenderGeom ( appendToRoot ( "assets/shaders/wave_system1D_render.geom" ).c_str () );
	GLShader shdRenderFrag ( appendToRoot ( "assets/shaders/wave_system1D_render.frag" ).c_str () );
	GLProgram progRender ( shdRenderVert, shdRenderGeom, shdRenderFrag );

	GLuint dummy_vao;
	glCreateVertexArrays ( 1, &dummy_vao );
	glBindVertexArray ( dummy_vao );

	GLuint renderHandle = progRender.getHandle ();

#ifndef NDEBUG
	const GLchar* kLabelRenderProg = "WaveSystemRenderProgram";
	glObjectLabel ( GL_PROGRAM, renderHandle, 0, kLabelRenderProg );

#endif

	g_pWaveSystem = new WaveSystem1D ( g_dt, g_wave_speed, g_num_points );
	g_pWaveSystem->initializeVertices ( initVerticesTriangleWave );
	g_pWaveSystem->updateSystem ();

	glEnable ( GL_CULL_FACE );
	glEnable ( GL_DEPTH_TEST );
	glEnable ( GL_BLEND );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );

	glfwSetCursorPosCallback (
		app.getWindow (),
		[] ( auto* window, double x, double y )
		{
			int width, height;
			glfwGetFramebufferSize ( window, &width, &height );
			mouseState.pos.x = static_cast< float >( x / width );
			mouseState.pos.y = static_cast< float >( y / height );
		}
	);

	glfwSetMouseButtonCallback (
		app.getWindow (),
		[] ( auto* window, int button, int action, int mods )
		{
			if ( button == GLFW_MOUSE_BUTTON_LEFT )
			{
				mouseState.pressedLeft = action == GLFW_PRESS;
			}
		}
	);

	glfwSetKeyCallback (
		app.getWindow (),
		[] ( auto* window, int key, int scancode, int action, int mods )
		{
			const bool pressed = action != GLFW_RELEASE;
			if ( key == GLFW_KEY_ESCAPE && pressed )
			{
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
			}
			if ( key == GLFW_KEY_P && pressed )
			{
				g_paused = !g_paused;
			}
			if ( key == GLFW_KEY_PERIOD && pressed )
			{
				g_wave_speed *= 1.01f;
				g_pWaveSystem->setWaveSpeed ( g_wave_speed );
			}
			if ( key == GLFW_KEY_COMMA && pressed )
			{
				g_wave_speed *= 0.99f;
				g_pWaveSystem->setWaveSpeed ( g_wave_speed );
			}
			if ( key == GLFW_KEY_RIGHT_BRACKET )
			{
				g_dt *= 1.01f;
				g_pWaveSystem->setTimeStep ( g_dt );
			}
			if ( key == GLFW_KEY_LEFT_BRACKET && pressed )
			{
				g_dt *= 0.99f;
				g_pWaveSystem->setTimeStep ( g_dt );
			}
			if ( key == GLFW_KEY_3 && pressed )
			{
				eProjectMode_current = eProjectMode::eProjectMode_None;
			}
			if ( key == GLFW_KEY_2 && pressed )
			{
				eProjectMode_current = eProjectMode::eProjectMode_Persp;
			}
			if ( key == GLFW_KEY_1 && pressed )
			{
				eProjectMode_current = eProjectMode::eProjectMode_Ortho;
			}
			if ( key == GLFW_KEY_0 && pressed )
			{
				eViewMode_current = eViewMode::eViewMode_None;
			}
			if ( key == GLFW_KEY_9 && pressed )
			{
				eViewMode_current = eViewMode::eViewMode_LookAt;
			}
			if ( key == GLFW_KEY_M && pressed )
			{
				g_box_percentage = std::min ( g_box_percentage + 0.01f, 1.0f );
			}
			if ( key == GLFW_KEY_N && pressed )
			{
				g_box_percentage = std::max ( g_box_percentage - 0.01f, 0.01f );
			}
			if ( key == GLFW_KEY_U && pressed )
			{
				if ( g_paused )
				{
					g_pWaveSystem->updateSystem ();
				}
			}
		}
	);

	int width, height;
	glfwGetFramebufferSize ( app.getWindow (), &width, &height );
	g_proj = glm::ortho ( 0, width, 0, height );
	g_view = mat4 ( 1.0f );
	g_model = mat4 ( 1.0f );

	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		glfwGetFramebufferSize ( app.getWindow (), &width, &height );

		const float aspect = width / static_cast< float >( height );

		glBindFramebuffer ( GL_FRAMEBUFFER, 0 );
		glViewport ( 0, 0, width, height );
		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		if ( !g_paused )
		{
			g_pWaveSystem->updateSystem ();
		}

		switch ( eProjectMode_current )
		{
		case eProjectMode::eProjectMode_None:
			g_proj = mat4 ( 1.0f );
			break;
		case eProjectMode::eProjectMode_Ortho:
			g_proj = glm::ortho ( 0, width, 0, height );
			break;
		case eProjectMode::eProjectMode_Persp:
			g_proj = glm::perspective ( 45.0f, aspect, 0.1f, 100.0f );
			break;
		default:
			break;
		}

		switch ( eViewMode_current )
		{
		case eViewMode::eViewMode_None:
			g_view = mat4 ( 1.0f );
			break;
		case eViewMode::eViewMode_LookAt:
			g_view = glm::lookAt ( kEyePosVec, kLookAtVec, kUpVec );
			break;
		default:
			break;
		}

		progRender.useProgram ();

		GLuint readHandle = g_pWaveSystem->getReadBufferHandle ();

		glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kIdxBind_WaveVerticesIn, readHandle );

		glUniform1f ( kIdxuniLoc_uBoxPercentage, g_box_percentage );
		glUniformMatrix4fv ( kIdxUniLoc_uProjectionMatrix, 1, GL_FALSE, glm::value_ptr ( g_proj ) );
		glUniformMatrix4fv ( kIdxUniLoc_uModelMatrix, 1, GL_FALSE, glm::value_ptr ( g_model ) );
		glUniformMatrix4fv ( kIdxUniLoc_uViewMatrix, 1, GL_FALSE, glm::value_ptr ( g_view ) );

		glDrawArrays ( GL_POINTS, 0, g_num_points );

		glUseProgram ( 0 );

		glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kIdxBind_WaveVerticesIn, 0 );

		app.swapBuffers ();
	}

	if ( g_pWaveSystem )
	{
		delete g_pWaveSystem;
		g_pWaveSystem = nullptr;
	}

	return 0;
}