#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <jcCommonFramework/Bitmap.h>
#include <jcCommonFramework/UtilsMath.h>
#include <jcCommonFramework/Camera.h>
#include <jcCommonFramework/scene/VtxData.h>

#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
#include <jcGLframework/GLTexture.h>
#include <jcGLframework/GLFramebuffer.h>
#include <jcGLframework/LineCanvasGL.h>
#include <jcGLframework/UtilsGLImGui.h>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 light;
	vec4 cameraPos;
	vec4 lightAngles; // cos(inner), cos(outer)
	vec4 lightPos;
};

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

CameraPositioner_FirstPerson positioner ( vec3 ( 0.0f, 6.0f, 11.0f ), vec3 ( 0.0f, 4.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
Camera camera ( positioner );

bool g_RotateModel = true;

float g_LightAngle = 60.0f;  // field of view angle
float g_LightInnerAngle = 10.0f; // light's inner cone
float g_LightNear = 1.0f;  
float g_LightFar = 20.0f;

float g_LightDist = 12.0f;
float g_LightXAngle = -1.0f;  // rotation about X axis
float g_LightYAngle = -2.0f;  // rotation about Y axis

int main ()
{
	GLFWApp app;

	GLShader shdGridVert ( appendToRoot ( "assets/shaders/GL08_grid.vert" ).c_str() );
	GLShader shdGridFrag ( appendToRoot ( "assets/shaders/GL08_grid.frag" ).c_str() );
	GLProgram progGrid ( shdGridVert, shdGridFrag );

	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );

	GLBuffer perFrameDataBuffer ( kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle (), 0, kUniformBufferSize );

	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	GLShader shdModelVert ( appendToRoot ( "assets/shaders/GL08_scene.vert" ).c_str () );
	GLShader shdModelFrag ( appendToRoot ( "assets/shaders/GL08_scene.frag" ).c_str () );
	GLProgram progModel ( shdModelVert, shdModelFrag );

	GLShader shdShadowVert ( appendToRoot ( "assets/shaders/GL08_shadow.vert" ).c_str () );
	GLShader shdShadowFrag ( appendToRoot ( "assets/shaders/GL08_shadow.frag" ).c_str () );
	GLProgram progShadowMap ( shdShadowVert, shdShadowFrag );

	// 1. Duck
	GLMeshPVP mesh ( appendToRoot ( "assets/models/rubber_duck/scene.gltf" ).c_str () );
	GLTexture texAlbedoDuck ( GL_TEXTURE_2D, appendToRoot ( "assets/models/rubber_duck/textures/Duck_baseColor.png" ).c_str () );

	// 2. Plane
	const std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };
	const std::vector<VertexData> vertices = {
		{.pos = vec3 ( -2, -2, 0 ), .n = vec3 ( 0, 0, 1 ), .tc = vec2 ( 0, 0 ) },
		{.pos = vec3 ( -2, +2, 0 ), .n = vec3 ( 0, 0, 1 ), .tc = vec2 ( 0, 1 ) },
		{.pos = vec3 ( +2, +2, 0 ), .n = vec3 ( 0, 0, 1 ), .tc = vec2 ( 1, 1 ) },
		{.pos = vec3 ( +2, -2, 0 ), .n = vec3 ( 0, 0, 1 ), .tc = vec2 ( 1, 0 ) }
	};

	GLMeshPVP plane ( indices, vertices.data (), uint32_t ( sizeof ( VertexData ) * vertices.size () ) );
	GLTexture texAlbedoPlane ( GL_TEXTURE_2D, appendToRoot ( "assets/images/sampleSTB.jpg" ).c_str () );

	const std::vector<GLMeshPVP*> meshesToDraw = { &mesh, &plane };

	// shadow map
	GLFramebuffer shadowMap ( 1024, 1024, GL_RGBA8, GL_DEPTH_COMPONENT24 );

	// model matrices
	const mat4 m ( 1.0f );
	GLBuffer modelMatrices ( sizeof ( mat4 ), value_ptr ( m ), GL_DYNAMIC_STORAGE_BIT );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, 2, modelMatrices.getHandle () );

	glfwSetCursorPosCallback (
		app.getWindow (),
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
		app.getWindow (),
		[]( auto* window, int button, int action, int mods )
		{
			auto& io = ImGui::GetIO ();
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			io.MouseDown[idx] = action == GLFW_PRESS;

			if ( !io.WantCaptureMouse )
			{
				if ( button == GLFW_MOUSE_BUTTON_LEFT )
				{
					mouseState.pressedLeft = action == GLFW_PRESS;
				}
			}
		}
	);

	positioner.maxSpeed_ = 5.0f;

	glfwSetKeyCallback (
		app.getWindow (),
		[]( GLFWwindow* window, int key, int scancode, int action, int mods )
		{
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
			else
				positioner.movement_.fastSpeed_ = false;
			if ( key == GLFW_KEY_SPACE )
				positioner.setUpVector ( vec3 ( 0.0f, 1.0f, 0.0f ) );
		}
	);

	double timeStamp = glfwGetTime ();
	float deltaSeconds = 0.0f;
	float angle = 0.0f;

	ImGuiGLRenderer rendererUI;
	CanvasGL canvas;

	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		positioner.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft );

		const double newTimeStamp = glfwGetTime ();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		int width, height;
		glfwGetFramebufferSize ( app.getWindow (), &width, &height );
		const float ratio = width / (float)height;

		if ( g_RotateModel ) angle += deltaSeconds;

		// 0. Calculate light parameters
		const mat4 rotY = glm::rotate ( mat4 ( 1.0f ), g_LightYAngle, vec3 ( 0, 1, 0 ) );
		const mat4 rotX = glm::rotate ( mat4 ( 1.0f ), g_LightXAngle, vec3 ( 1, 0, 0 ) );
		const vec4 lightPos = rotX * glm::vec4 ( 0, 0, g_LightDist, 1.0f );
		const mat4 lightProj = glm::perspective ( glm::radians ( g_LightAngle ), 1.0f, g_LightNear, g_LightFar );
		const mat4 lightView = glm::lookAt ( vec3 ( lightPos ), vec3 ( 0 ), vec3 ( 0, 1, 0 ) );

		glEnable ( GL_DEPTH_TEST );
		glDisable ( GL_BLEND );

		// 1. Render shadow map
		{
			const PerFrameData perFrameData = { .view = lightView, .proj = lightProj, .cameraPos = vec4 ( camera.getPosition (), 1.0f ) };
			glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, kUniformBufferSize, &perFrameData );
			shadowMap.bind ();
			glClearNamedFramebufferfv ( shadowMap.getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4 ( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
			glClearNamedFramebufferfi ( shadowMap.getHandle (), GL_DEPTH_STENCIL, 0, 1.0f, 0 );
			progShadowMap.useProgram ();
			for ( const auto& m : meshesToDraw )
			{
				m->drawElements ();
			}
			shadowMap.unbind ();
		}

		// 2. Render scene
		glViewport ( 0, 0, width, height );
		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		const mat4 proj = glm::perspective ( 45.0f, ratio, 0.5f, 5000.0f );
		const mat4 view = camera.getViewMatrix ();

		const PerFrameData perFrameData = {
			.view = view,
			.proj = proj,
			.light = lightProj * lightView,
			.cameraPos = vec4 ( camera.getPosition (), 1.0f ),
			.lightAngles = vec4 ( cosf ( glm::radians ( 0.5f * g_LightAngle ) ), cosf ( glm::radians ( 0.5f * (g_LightAngle - g_LightInnerAngle) ) ), 1.0f, 1.0f ),
			.lightPos = lightPos
		};

		glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, kUniformBufferSize, &perFrameData );

		const mat4 scale = glm::scale ( mat4 ( 1.0f ), vec3 ( 3.0f ) );
		const mat4 rot = glm::rotate ( mat4 ( 1.0f ), glm::radians ( -90.0f ), vec3 ( 1.0f, 0.0f, 0.0 ) );
		const mat4 pos = glm::translate ( mat4 ( 1.0f ), vec3 ( 0.0f, 0.0f, +1.0f ) );
		const mat4 m = glm::rotate ( scale * rot * pos, angle, vec3 ( 0.0f, 0.0f, 1.0f ) );
		glNamedBufferSubData ( modelMatrices.getHandle (), 0, sizeof ( mat4 ), glm::value_ptr ( m ) );

		const GLuint textures[] = { texAlbedoDuck.getHandle (), texAlbedoPlane.getHandle () };
		glBindTextureUnit ( 1, shadowMap.getTextureDepth ().getHandle () );
		progModel.useProgram ();
		for ( size_t i = 0; i != meshesToDraw.size (); i++ )
		{
			glBindTextureUnit ( 0, textures[i] );
			meshesToDraw[i]->drawElements ();
		}

		glEnable ( GL_BLEND );
		progGrid.useProgram ();
		glDrawArraysInstancedBaseInstance ( GL_TRIANGLES, 0, 6, 1, 0 );

		renderCameraFrustrumGL ( canvas, lightView, lightProj, vec4 ( 0.0f, 1.0f, 0.0f, 1.0f ) );
		canvas.flush ();

		ImGuiIO& io = ImGui::GetIO ();
		io.DisplaySize = ImVec2 ( (float)width, (float)height );
		ImGui::NewFrame ();

		ImGui::Begin ( "Control", nullptr );
		ImGui::Checkbox ( "Rotate model", &g_RotateModel );
		ImGui::Text ( "Light parameters", nullptr );
		ImGui::SliderFloat ( "Proj::Light angle", &g_LightAngle, 15.0f, 170.0f );
		ImGui::SliderFloat ( "Proj::Light inner angle", &g_LightInnerAngle, 1.0f, 15.0f );
		ImGui::SliderFloat ( "Proj::Near", &g_LightNear, 0.1f, 5.0f );
		ImGui::SliderFloat ( "Proj::Far", &g_LightFar, 0.1f, 100.0f );
		ImGui::SliderFloat ( "Pos::Dist", &g_LightDist, 0.5f, 100.0f );
		ImGui::SliderFloat ( "Pos::AngleX", &g_LightXAngle, -3.15f, +3.15f );
		ImGui::SliderFloat ( "Pos::AngleY", &g_LightYAngle, -3.15f, +3.15f );
		ImGui::End ();

		imguiTextureWindowGL ( "Color", shadowMap.getTextureColor ().getHandle () );
		imguiTextureWindowGL ( "Depth", shadowMap.getTextureDepth ().getHandle () );

		ImGui::Render ();
		rendererUI.render ( width, height, ImGui::GetDrawData () );

		app.swapBuffers ();
	}

	return 0;
}