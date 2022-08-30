#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
#include <jcGLframework/LineCanvasGL.h>
#include <jcGLframework/UtilsGLImGui.h>
#include <jcCommonFramework/ResourceString.h>
#include <jcCommonFramework/Camera.h>
#include <jcCommonFramework/UtilsFPS.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "WaveSystem1D.h"

#include <iostream>
#include <istream>
#include <fstream>
#include <sstream>
#include <iomanip>

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;

constexpr GLuint kIdxBind_UpdateRead = 0;
constexpr GLuint kIdxBind_UpdateWrite = 1;
constexpr GLuint kIdxBind_WaveVerticesIn = 1;
constexpr GLuint kIdxBind_VBO = 0;
constexpr GLuint kIdxBind_PerFrameData = 7;
constexpr GLuint kIdxAttrib_aHeightVelocity = 0;

constexpr vec3 kLookAtVec = vec3 ( 0.0f, 0.0f, 0.0f );
constexpr vec3 kEyePosVec = vec3 ( 0.0f, 0.0f, -2.0f );
constexpr vec3 kUpVec = vec3 ( 0.0f, 1.0f, 0.0f );

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 model;
	vec4 cameraPos;
	GLuint vertCount;
	float boxPercentage;
};

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

CameraPositioner_FirstPerson positioner ( kEyePosVec, kLookAtVec, kUpVec );
Camera camera ( positioner );

FramesPerSecondCounter fpsCounter;

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
bool g_copy_on_update = false;
mat4 g_proj = mat4 ( 1.0f );
mat4 g_view = mat4 ( 1.0f );
mat4 g_model = mat4 ( 1.0f );
mat4 g_frustum_view = mat4 ( 1.0f );
float g_box_percentage = 0.9f;
uint32_t g_num_points = 256;
float g_dt = 0.1f;
float g_wave_speed = 1.0f;

GLuint g_vbo = 0;
GLuint g_ubo = 0;

enum class eDumpBuffer : uint8_t
{
	eDumpBuffer_None = 0x00,
	eDumpBuffer_WaveSSBO = 0x01,
	eDumpBuffer_VBO = 0x02,
	eDumpBuffer_UBO = 0x03,
	eDumpBuffer_Count = 0x04
} eDumpBuffer_current;

WaveSystem1D* g_pWaveSystem = nullptr;

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

std::vector<float> initVerticesBuffer ( const std::vector<Wave1DVert>& initialState )
{
	const size_t vertCount = initialState.size ();
	std::vector<float> bufferData;
	bufferData.resize ( vertCount * 2 );
	for ( size_t i = 0; i < vertCount; i++ )
	{
		bufferData [ 2 * i + 0] = initialState [ i ].height;
		bufferData [ 2 * i + 1 ] = initialState [ i ].velocity;
	}
	return bufferData;
}

void copyBuffers ( GLuint readBuffer, GLuint writeBuffer, GLsizeiptr size )
{
	glCopyNamedBufferSubData ( readBuffer, writeBuffer, 0, 0, size );
}

void dumpBuffer ( std::ostream& os, GLuint buff, GLsizeiptr size )
{
	GLuint tmpBuff;
	glCreateBuffers ( 1, &tmpBuff );
	glNamedBufferStorage ( tmpBuff, size, nullptr, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT );
	copyBuffers ( buff, tmpBuff, size );
	float* ptr = (float *)glMapNamedBuffer ( tmpBuff, GL_READ_ONLY );
	const GLsizeiptr count = static_cast< GLsizeiptr >( size / sizeof ( float ) );

	GLchar objBuff [ 64 ];
	GLsizei buffLen;
	glGetObjectLabel ( GL_BUFFER, buff, 64, &buffLen, objBuff );
	std::string objName ( objBuff, buffLen );
	os << "Dumping Buffer " << objName << " size " << size << std::endl;
	os << std::setprecision ( 5 );
	for ( GLsizeiptr i = 0; i < count; ++i )
	{
		os << ( *ptr++ );
		if ( i % 60 )  // new line every 60 values
		{
			os << std::endl;
		}
		else
		{
			os << ",";
		}
	}

	os << std::endl << "Done." << std::endl;
	glDeleteBuffers ( 1, &tmpBuff );
}

int main ( int argc, char** argv )
{
	GLFWApp app;

	eProjectMode_current = eProjectMode::eProjectMode_Ortho;
	eViewMode_current = eViewMode::eViewMode_None;
	eDumpBuffer_current = eDumpBuffer::eDumpBuffer_VBO;

	GLShader shdRenderVert ( appendToRoot ( "assets/shaders/wave_system1D_render_alt.vert" ).c_str () );
	GLShader shdRenderGeom ( appendToRoot ( "assets/shaders/wave_system1D_render_alt.geom" ).c_str () );
	GLShader shdRenderFrag ( appendToRoot ( "assets/shaders/wave_system1D_render_alt.frag" ).c_str () );
	GLProgram progRender ( shdRenderVert, shdRenderGeom, shdRenderFrag );

	GLShader shdGridVert ( appendToRoot ( "assets/shaders/GL10_grid.vert" ).c_str () );
	GLShader shdGridFrag ( appendToRoot ( "assets/shaders/GL10_grid.frag" ).c_str () );
	GLProgram progGrid ( shdGridVert, shdGridFrag );

	GLuint renderHandle = progRender.getHandle ();
	GLuint gridHandle = progGrid.getHandle ();

#ifndef NDEBUG
	const GLchar* kLabelRenderProg = "WaveSystemRenderProgram";
	const GLchar* kLabelGridProg = "GridRenderProgram";
	glObjectLabel ( GL_PROGRAM, renderHandle, 0, kLabelRenderProg );
	glObjectLabel ( GL_PROGRAM, gridHandle, 0, kLabelGridProg );
#endif

	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );
	GLBuffer perFrameDataBuffer ( kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	g_ubo = perFrameDataBuffer.getHandle ();

#ifndef NDEBUG
	
	const GLchar* kLabelUBO = "PerFrameDataBuffer";
	glObjectLabel ( GL_BUFFER, g_ubo, 0, kLabelUBO );
#endif
	glBindBufferRange ( GL_UNIFORM_BUFFER, kIdxBind_PerFrameData, perFrameDataBuffer.getHandle (), 0, kUniformBufferSize );

	g_pWaveSystem = new WaveSystem1D ( g_dt, g_wave_speed, g_num_points );
	g_pWaveSystem->initializeVertices ( initVerticesTriangleWave );

	const std::vector<Wave1DVert>& waveSystemInitialState = g_pWaveSystem->getInitialState ();
	std::vector<float> bufferData = initVerticesBuffer ( waveSystemInitialState );

	const GLsizei kVertexBufferSize = 2 * g_num_points * sizeof ( float );
	const GLsizei kVertexBufferStride = 2 * sizeof ( float );
	
	GLuint vao, vbo;
	glCreateVertexArrays ( 1, &vao );

#ifndef NDEBUG
	const GLchar* kLabelVAO = "VAO";
	glObjectLabel ( GL_VERTEX_ARRAY, vao, 0, kLabelVAO );
#endif

	glBindVertexArray ( vao );

	glCreateBuffers ( 1, &vbo );

	g_vbo = vbo;

#ifndef NDEBUG
	const GLchar* kLabelVBO = "VBO";
	glObjectLabel ( GL_BUFFER, vbo, 0, kLabelVBO );
#endif

	glNamedBufferStorage ( vbo, kVertexBufferSize, bufferData.data (), GL_DYNAMIC_STORAGE_BIT );

	glVertexArrayAttribBinding ( vao, kIdxAttrib_aHeightVelocity, kIdxBind_VBO );
	glVertexArrayVertexBuffer ( vao, kIdxBind_VBO, vbo, 0, kVertexBufferStride );
	glVertexArrayAttribFormat ( vao, kIdxAttrib_aHeightVelocity, 2, GL_FLOAT, GL_FALSE, 0 );
	glEnableVertexArrayAttrib ( vao, kIdxAttrib_aHeightVelocity );

	glEnable ( GL_DEPTH_TEST );
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
			ImGui::GetIO ().MousePos = ImVec2 ( ( float ) x, ( float ) y );
		}
	);

	glfwSetMouseButtonCallback (
		app.getWindow (),
		[] ( auto* window, int button, int action, int mods )
		{
			auto& io = ImGui::GetIO ();
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			io.MouseDown [ idx ] = action == GLFW_PRESS;
			if ( !io.WantCaptureMouse )
			{
				if ( button == GLFW_MOUSE_BUTTON_LEFT )
				{
					mouseState.pressedLeft = action == GLFW_PRESS;
				}
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
			if ( key == GLFW_KEY_C )
			{
				g_copy_on_update = !g_copy_on_update;
			}
			if ( key == GLFW_KEY_W )
				positioner.movement_.forward_ = pressed;
			if ( key == GLFW_KEY_S )
				positioner.movement_.backward_ = pressed;
			if ( key == GLFW_KEY_A )
				positioner.movement_.left_ = pressed;
			if ( key == GLFW_KEY_D )
				positioner.movement_.right_ = pressed;
			if ( key == GLFW_KEY_Q )
				positioner.movement_.up_ = pressed;
			if ( key == GLFW_KEY_Z )
				positioner.movement_.down_ = pressed;
			if ( key == GLFW_KEY_SPACE )
				positioner.setUpVector ( vec3 ( 0.0f, 1.0f, 0.0f ) );
			if ( key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT )
				positioner.movement_.fastSpeed_ = pressed;		
			if ( key == GLFW_KEY_O && pressed )
			{
				GLuint handle;
				switch ( eDumpBuffer_current )
				{
				case eDumpBuffer::eDumpBuffer_None:
					break;
				case eDumpBuffer::eDumpBuffer_WaveSSBO:
					handle = g_pWaveSystem->getReadBufferHandle ();
					dumpBuffer ( std::cout, handle, g_num_points * sizeof ( Wave1DVert ) );
					break;
				case eDumpBuffer::eDumpBuffer_VBO:
					handle = g_vbo;
					dumpBuffer ( std::cout, handle, g_num_points * 2 * sizeof ( float ) );
					break;
				case eDumpBuffer::eDumpBuffer_UBO:
					handle = g_ubo;
					dumpBuffer ( std::cout, handle, sizeof ( PerFrameData ) );
					break;
				case eDumpBuffer::eDumpBuffer_Count:
					break;
				default:
					break;
				}
			}
			if ( key == GLFW_KEY_I && pressed )
			{
				switch ( eDumpBuffer_current )
				{
				case eDumpBuffer::eDumpBuffer_None:
					eDumpBuffer_current = eDumpBuffer::eDumpBuffer_WaveSSBO;
					break;
				case eDumpBuffer::eDumpBuffer_WaveSSBO:
					eDumpBuffer_current = eDumpBuffer::eDumpBuffer_VBO;
					break;
				case eDumpBuffer::eDumpBuffer_VBO:
					eDumpBuffer_current = eDumpBuffer::eDumpBuffer_UBO;
					break;
				case eDumpBuffer::eDumpBuffer_UBO:
					eDumpBuffer_current = eDumpBuffer::eDumpBuffer_None;
					break;
				case eDumpBuffer::eDumpBuffer_Count:
					eDumpBuffer_current = eDumpBuffer::eDumpBuffer_None;
					break;
				default:
					eDumpBuffer_current = eDumpBuffer::eDumpBuffer_None;
					break;
				}
			}
		}
	);


	fpsCounter.printFPS_ = false;

	positioner.maxSpeed_ = 1.0f;

	ImGuiGLRenderer rendererUI;
	CanvasGL canvas;

	int width, height;
	glfwGetFramebufferSize ( app.getWindow (), &width, &height );
	g_proj = glm::ortho ( 0, width, 0, height );
	g_view = mat4 ( 1.0f );
	g_model = mat4 ( 1.0f );
	
	g_pWaveSystem->updateSystem ();

	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		fpsCounter.tick ( app.getDeltaSeconds () );
		positioner.update ( app.getDeltaSeconds (), mouseState.pos, mouseState.pressedLeft );

		glfwGetFramebufferSize ( app.getWindow (), &width, &height );

		glBindFramebuffer ( GL_FRAMEBUFFER, 0 );
		glViewport ( 0, 0, width, height );
		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		const float aspect = width / static_cast< float >( height );
		const float scaleFactor = 0.25f * static_cast< float >( std::max ( width, height ) );

		const mat4 proj = glm::perspective ( 45.0f, aspect, 0.1f, 100.0f );
		const mat4 view = camera.getViewMatrix ();
		const vec3 camPosition = camera.getPosition ();
		const mat4 model = glm::scale ( mat4 ( 1.0f ), vec3 ( scaleFactor, scaleFactor, scaleFactor ) );

		const PerFrameData perFrameData = {
			.view = view,
			.proj = proj,
			.model = model,
			.cameraPos = vec4 ( camPosition, 1.0f ),
			.vertCount = g_num_points,
			.boxPercentage = g_box_percentage
		};

		glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, kUniformBufferSize, &perFrameData );

		g_frustum_view = camera.getViewMatrix ();

		vec4 frustumPlanes [ 6 ];
		getFrustumPlanes ( proj* g_frustum_view, frustumPlanes );
		vec4 frustumCorners [ 8 ];
		getFrustumCorners ( proj* g_frustum_view, frustumCorners );

		if ( !g_paused )
		{
			g_pWaveSystem->updateSystem ();
		}

		if ( g_copy_on_update )
		{
			GLuint readHandle = g_pWaveSystem->getReadBufferHandle ();
			copyBuffers ( readHandle, vbo, ( GLsizeiptr ) ( g_num_points * sizeof ( Wave1DVert ) ) );
		}

		glDisable ( GL_BLEND );
		glEnable ( GL_DEPTH_TEST );

		progRender.useProgram ();

		glBindVertexArray ( vao );
		glDrawArrays ( GL_POINTS, 0, g_num_points );

//		glUseProgram ( 0 );
//		glBindVertexArray ( 0 );

		glEnable ( GL_BLEND );

		progGrid.useProgram ();
		glDrawArraysInstancedBaseInstance ( GL_TRIANGLES, 0, 6, 1, 0 );

		renderCameraFrustumGL ( canvas, g_frustum_view, proj, vec4 ( 1.0f, 1.0f, 0.0f, 1.0f ), 100 );

		canvas.flush ();

		float fpsCurrent = fpsCounter.getFPS ();

		ImGuiIO& io = ImGui::GetIO ();
		io.DisplaySize = ImVec2 ( ( float ) width, ( float ) height );
		ImGui::NewFrame ();
		ImGui::Begin ( "Control", nullptr );
		ImGui::Text ( "Draw:" );
		ImGui::Checkbox ( "Pause", &g_paused );
		ImGui::Checkbox ( "Copy On Update", &g_copy_on_update );
		ImGui::Separator ();
		ImGui::Text ( "FPS: %f", fpsCurrent );
		ImGui::End ();
		ImGui::Render ();
		rendererUI.render ( width, height, ImGui::GetDrawData () );

		app.swapBuffers ();
	}

	if ( g_pWaveSystem )
	{
		delete g_pWaveSystem;
		g_pWaveSystem = nullptr;
	}

	return 0;
}