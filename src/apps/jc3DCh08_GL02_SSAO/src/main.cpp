#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
#include <jcGLframework/GLSceneData.h>
#include <jcGLframework/GLFramebuffer.h>
#include <jcGLframework/GLMesh.h>
#include <jcGLframework/UtilsGLImGui.h>
#include <jcCommonFramework/UtilsMath.h>
#include <jcCommonFramework/Camera.h>
#include <jcCommonFramework/scene/VtxData.h>
#include <jcCommonFramework/ResourceString.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

struct SSAOParams
{
	float scale_ = 1.0f;
	float bias_ = 0.2f;
	float zNear_ = 0.1f;
	float zFar_ = 1000.0f;
	float radius_ = 0.2f;
	float attScale_ = 1.0f;
	float distScale_ = 0.5f;
} g_SSAOParams;

static_assert(sizeof ( SSAOParams ) <= sizeof ( PerFrameData ));

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

CameraPositioner_FirstPerson positioner ( vec3 ( -10.0f, 3.0f, 3.0f ), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
Camera camera ( positioner );

bool g_EnableSSAO = true;
bool g_EnableBlur = true;

int main ()
{
	GLFWApp app;

	GLShader shdGridVert ( appendToRoot ( "assets/shaders/GL08_grid.vert" ).c_str());
	GLShader shdGridFrag ( appendToRoot ( "assets/shaders/GL08_grid.frag" ).c_str () );
	GLProgram progGrid ( shdGridVert, shdGridFrag );

	GLShader shdMeshVert ( appendToRoot ( "assets/shaders/GL08_mesh.vert" ).c_str () );
	GLShader shdMeshFrag ( appendToRoot ( "assets/shaders/GL08_mesh.frag" ).c_str () );
	GLProgram progMesh ( shdMeshVert, shdMeshFrag );

	GLShader shdFullScreenQuadVert ( appendToRoot ( "assets/shaders/GL08_FullScreenQuad.vert" ).c_str () );

	GLShader shdSSAOFrag ( appendToRoot ( "assets/shaders/GL08_SSAO.frag" ).c_str () );
	GLShader shdCombineSSAOFrag ( appendToRoot ( "assets/shaders/GL08_SSAO_combine.frag" ).c_str () );
	GLProgram progSSAO ( shdFullScreenQuadVert, shdSSAOFrag );
	GLProgram progCombineSSAO ( shdFullScreenQuadVert, shdCombineSSAOFrag );

	GLShader shdBlurXFrag ( appendToRoot ( "assets/shaders/GL08_BlurX.frag" ).c_str () );
	GLShader shdBlurYFrag ( appendToRoot ( "assets/shaders/GL08_BlurY.frag" ).c_str () );
	GLProgram progBlurX ( shdFullScreenQuadVert, shdBlurXFrag );
	GLProgram progBlurY ( shdFullScreenQuadVert, shdBlurYFrag );

	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );

	GLBuffer perFrameDataBuffer ( kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle (), 0, kUniformBufferSize );

	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable ( GL_DEPTH_TEST );

	GLTexture rotationPattern ( GL_TEXTURE_2D, appendToRoot ( "assets/images/rot_texture.bmp" ).c_str () );

	GLSceneData sceneData1 ( appendToRoot ( "assets/meshes/test.meshes" ).c_str (), appendToRoot ( "assets/meshes/test.scene" ).c_str (), appendToRoot ( "assets/meshes/test.materials" ).c_str () );
	GLSceneData sceneData2 ( appendToRoot ( "assets/meshes/test2.meshes" ).c_str (), appendToRoot ( "assets/meshes/test2.scene" ).c_str (), appendToRoot ( "assets/meshes/test2.materials" ).c_str () );

	GLMesh mesh1 ( sceneData1 );
	GLMesh mesh2 ( sceneData2 );

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
				if ( button == GLFW_MOUSE_BUTTON_LEFT )
					mouseState.pressedLeft = action == GLFW_PRESS;
		}
	);

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
			if ( key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT )
				positioner.movement_.fastSpeed_ = pressed;
			if ( key == GLFW_KEY_SPACE )
				positioner.setUpVector ( vec3 ( 0.0f, 1.0f, 0.0f ) );
		}
	);

	positioner.maxSpeed_ = 1.0f;

	double timeStamp = glfwGetTime ();
	float deltaSeconds = 0.0f;

	// offscreen render targets
	int width, height;
	glfwGetFramebufferSize ( app.getWindow (), &width, &height );
	GLFramebuffer framebuffer ( width, height, GL_RGBA8, GL_DEPTH_COMPONENT24 );
	GLFramebuffer ssao ( 1024, 1024, GL_RGBA8, 0 );
	GLFramebuffer blur ( 1024, 1024, GL_RGBA8, 0 );

	ImGuiGLRenderer rendererUI;

	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		positioner.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft );

		const double newTimeStamp = glfwGetTime ();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		int width, height;
		glfwGetFramebufferSize ( app.getWindow (), &width, &height );
		const float ratio = width / (float)height;

		glClearNamedFramebufferfv ( framebuffer.getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4 ( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		glClearNamedFramebufferfi ( framebuffer.getHandle (), GL_DEPTH_STENCIL, 0, 1.0f, 0 );

		const mat4 p = glm::perspective ( 45.0f, ratio, g_SSAOParams.zNear_, g_SSAOParams.zFar_ );
		const mat4 view = camera.getViewMatrix ();

		const PerFrameData perFrameData = {
			.view = view,
			.proj = p,
			.cameraPos = vec4 ( camera.getPosition (), 1.0f )
		};

		glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, kUniformBufferSize, &perFrameData );

		// 1. Render scene
		glDisable ( GL_BLEND );
		glEnable ( GL_DEPTH_TEST );
		framebuffer.bind ();
		// 1.1 Bistro
		progMesh.useProgram ();
		mesh1.draw ( sceneData1 );
		mesh2.draw ( sceneData2 );
		// 1.2 Grid
		glEnable ( GL_BLEND );
		progGrid.useProgram ();
		glDrawArraysInstancedBaseInstance ( GL_TRIANGLES, 0, 6, 1, 0 );
		framebuffer.unbind ();
		glDisable ( GL_DEPTH_TEST );

		// 2. Calculate SSAO
		glClearNamedFramebufferfv ( ssao.getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4 ( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, sizeof ( g_SSAOParams ), &g_SSAOParams );
		ssao.bind ();
		progSSAO.useProgram ();
		glBindTextureUnit ( 0, framebuffer.getTextureDepth ().getHandle () );
		glBindTextureUnit ( 1, rotationPattern.getHandle () );
		glDrawArrays ( GL_TRIANGLES, 0, 6 );
		ssao.unbind ();

		// 2.1 Blur SSAO
		if ( g_EnableBlur )
		{
			// Blur X
			blur.bind ();
			progBlurX.useProgram ();
			glBindTextureUnit ( 0, ssao.getTextureColor ().getHandle () );
			glDrawArrays ( GL_TRIANGLES, 0, 6 );
			blur.unbind ();
			// Blur Y
			ssao.bind ();
			progBlurY.useProgram ();
			glBindTextureUnit ( 0, blur.getTextureColor ().getHandle () );
			glDrawArrays ( GL_TRIANGLES, 0, 6 );
			ssao.unbind ();
		}

		// 3. Combine SSAO and the rendered scene
		glViewport ( 0, 0, width, height );
		if ( g_EnableSSAO )
		{
			progCombineSSAO.useProgram ();
			glBindTextureUnit ( 0, framebuffer.getTextureColor ().getHandle () );
			glBindTextureUnit ( 1, ssao.getTextureColor ().getHandle () );
			glDrawArrays ( GL_TRIANGLES, 0, 6 );
		} else
		{
			glBlitNamedFramebuffer ( framebuffer.getHandle (), 0, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR );
		}

		ImGuiIO& io = ImGui::GetIO ();
		io.DisplaySize = ImVec2 ( (float)width, (float)height );
		ImGui::NewFrame ();
		ImGui::Begin ( "Control", nullptr );
		ImGui::Checkbox ( "Enable SSAO", &g_EnableSSAO );
		ImGui::PushItemFlag ( ImGuiItemFlags_Disabled, !g_EnableSSAO );
		ImGui::PushStyleVar ( ImGuiStyleVar_Alpha, ImGui::GetStyle ().Alpha * g_EnableSSAO ? 1.0 : 0.2f );
		ImGui::Checkbox ( "Enable blur", &g_EnableBlur );
		ImGui::SliderFloat ( "SSAO scale", &g_SSAOParams.scale_, 0.0f, 2.0f );
		ImGui::SliderFloat ( "SSAO bias", &g_SSAOParams.bias_, 0.0f, 0.3f );
		ImGui::PopItemFlag ();
		ImGui::PopStyleVar ();
		ImGui::Separator ();
		ImGui::SliderFloat ( "SSAO radius", &g_SSAOParams.radius_, 0.05f, 0.5f );
		ImGui::SliderFloat ( "SSAO attenuation scale", &g_SSAOParams.attScale_, 0.5f, 1.5f );
		ImGui::SliderFloat ( "SSAO distance scale", &g_SSAOParams.distScale_, 0.0f, 1.0f );
		ImGui::End ();
		imguiTextureWindowGL ( "Color", framebuffer.getTextureColor ().getHandle () );
		imguiTextureWindowGL ( "Depth", framebuffer.getTextureDepth ().getHandle () );
		imguiTextureWindowGL ( "SSAO", ssao.getTextureColor ().getHandle () );
		ImGui::Render ();
		rendererUI.render ( width, height, ImGui::GetDrawData () );

		app.swapBuffers ();
	}

	return 0;
}