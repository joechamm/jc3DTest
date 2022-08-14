#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
#include <jcGLframework/GLSceneDataLazy.h>
#include <jcGLframework/GLFramebuffer.h>
#include <jcGLframework/LineCanvasGL.h>
#include <jcGLframework/UtilsGLImGui.h>
#include <jcGLframework/GLMeshIndirect.h>
#include <jcGLframework/GLSkyboxRenderer.h>
#include <jcCommonFramework/UtilsMath.h>
#include <jcCommonFramework/Camera.h>
#include <jcCommonFramework/scene/VtxData.h>
#include <jcCommonFramework/ResourceString.h>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 light = mat4 ( 0.0f );
	vec4 cameraPos;
};

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

CameraPositioner_FirstPerson positioner ( vec3 ( -10.0f, 3.0f, 3.0f ), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
Camera camera ( positioner );

bool g_DrawOpaque = true;
bool g_DrawTransparent = true;
bool g_DrawGrid = true;

int main ()
{
	GLFWApp app;

	GLShader shdGridVert ( appendToRoot ( "assets/shaders/GL10_grid.vert" ).c_str () );
	GLShader shdGridFrag ( appendToRoot ( "assets/shaders/GL10_grid.frag" ).c_str () );
	GLProgram progGrid ( shdGridVert, shdGridFrag );

	GLShader shdSceneVert ( appendToRoot ( "assets/shaders/GL10_scene_IBL.vert" ).c_str () );
	GLShader shdSceneFrag ( appendToRoot ( "assets/shaders/GL10_scene_IBL.frag" ).c_str () );
	GLProgram progScene ( shdSceneVert, shdSceneFrag );

	GLShader shdFragOIT ( appendToRoot ( "assets/shaders/GL10_mesh_oit.frag" ).c_str () );
	GLProgram progOIT ( shdSceneVert, shdFragOIT );

	GLShader shdFullScreenQuadVert ( appendToRoot ( "assets/shaders/GL10_FullScreenQuad.vert" ).c_str () );
	GLShader shdCombineOIT ( appendToRoot ( "assets/shaders/GL10_OIT.frag" ).c_str () );
	GLProgram progCombineOIT ( shdFullScreenQuadVert, shdCombineOIT );

	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );
	GLBuffer perFrameDataBuffer ( kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle (), 0, kUniformBufferSize );

	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable ( GL_DEPTH_TEST );

	GLSceneDataLazy sceneData ( appendToRoot ( "assets/meshes/bistro_all.meshes" ).c_str (), appendToRoot ( "assets/meshes/bistro_all.scene" ).c_str (), appendToRoot ( "assets/meshes/bistro_all.materials" ).c_str () );
	GLMeshIndirect mesh ( sceneData );

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
				if ( button == GLFW_MOUSE_BUTTON_LEFT )
					mouseState.pressedLeft = action == GLFW_PRESS;
		}
	);

	glfwSetKeyCallback (
		app.getWindow (),
		[] ( GLFWwindow* window, int key, int scancode, int action, int mods )
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

	GLSkyboxRenderer skybox;
	ImGuiGLRenderer rendererUI;

	GLIndirectBuffer meshesOpaque ( sceneData.shapes_.size () );
	GLIndirectBuffer meshesTransparent ( sceneData.shapes_.size () );

	auto isTransparent = [&sceneData] ( const DrawElementsIndirectCommand& c )
	{
		const auto mtlIndex = c.baseInstance_ & 0xFFFF;
		const auto& mtl = sceneData.materials_ [ mtlIndex ];
		return ( mtl.flags_ & sMaterialFlags_Transparent ) > 0;
	};

	mesh.bufferIndirect_.selectTo ( meshesOpaque, [&isTransparent] ( const DrawElementsIndirectCommand& c ) -> bool
		{
			return !isTransparent ( c );
		} );

	mesh.bufferIndirect_.selectTo ( meshesTransparent, [&isTransparent] ( const DrawElementsIndirectCommand& c ) -> bool
		{
			return isTransparent ( c );
		} );

	struct TransparentFragment
	{
		float R, G, B, A;
		float Depth;
		uint32_t Next;
	};

	int width, height;
	glfwGetFramebufferSize ( app.getWindow (), &width, &height );
	GLFramebuffer framebuffer ( width, height, GL_RGBA8, GL_DEPTH_COMPONENT24 );

	const uint32_t kMaxOITFragments = 32 * 1024 * 1024;
	const uint32_t kBufferIndex_TransparencyLists = kBufferIndex_Materials + 1;

	GLBuffer oitAtomicCounter ( sizeof ( uint32_t ), nullptr, GL_DYNAMIC_STORAGE_BIT );
	GLBuffer oitTransparencyLists ( sizeof ( TransparentFragment ) * kMaxOITFragments, nullptr, GL_DYNAMIC_STORAGE_BIT );

	GLTexture oitHeads ( GL_TEXTURE_2D, width, height, GL_R32UI );
	glBindImageTexture ( 0, oitHeads.getHandle (), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI );
	glBindBufferBase ( GL_ATOMIC_COUNTER_BUFFER, 0, oitAtomicCounter.getHandle () );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kBufferIndex_TransparencyLists, oitTransparencyLists.getHandle () );

	auto clearTransparencyBuffers = [&oitAtomicCounter, &oitHeads] ()
	{
		const uint32_t minusOne = 0xFFFFFFFF;
		const uint32_t zero = 0;
		glClearTexImage ( oitHeads.getHandle (), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &minusOne );
		glNamedBufferSubData ( oitAtomicCounter.getHandle (), 0, sizeof ( uint32_t ), &zero );
	};

	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		if ( sceneData.uploadLoadedTextures () )
			mesh.updateMaterialsBuffer ( sceneData );

		positioner.update ( app.getDeltaSeconds (), mouseState.pos, mouseState.pressedLeft );

		int width, height;
		glfwGetFramebufferSize ( app.getWindow (), &width, &height );
		const float ratio = width / ( float ) height;

		glViewport ( 0, 0, width, height );
		glClearNamedFramebufferfv ( framebuffer.getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4 ( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		glClearNamedFramebufferfi ( framebuffer.getHandle (), GL_DEPTH_STENCIL, 0, 1.0f, 0 );

		const mat4 proj = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );
		const mat4 view = camera.getViewMatrix ();
		PerFrameData perFrameData = { .view = view, .proj = proj, .light = mat4 ( 0.0f ), .cameraPos = vec4 ( camera.getPosition (), 1.0f ) };

		glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, kUniformBufferSize, &perFrameData );

		clearTransparencyBuffers ();

		// 1. Render scene
		framebuffer.bind ();
		glDisable ( GL_BLEND );
		glEnable ( GL_DEPTH_TEST );
		
		// 1.0 Cube map
		skybox.draw ();
		// 1.1 Bistro
		if ( g_DrawOpaque )
		{
			progScene.useProgram ();
			mesh.draw ( meshesOpaque.drawCommands_.size (), &meshesOpaque );
		}

		if ( g_DrawGrid )
		{
			glEnable ( GL_BLEND );
			progGrid.useProgram ();
			glDrawArraysInstancedBaseInstance ( GL_TRIANGLES, 0, 6, 1, 0 );
			glDisable ( GL_BLEND );
		}

		if ( g_DrawTransparent )
		{
			glDepthMask ( GL_FALSE );
			glColorMask ( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
			progOIT.useProgram ();
			mesh.draw ( meshesTransparent.drawCommands_.size (), &meshesTransparent );
			glFlush ();
			glMemoryBarrier ( GL_SHADER_STORAGE_BARRIER_BIT );
			glDepthMask ( GL_TRUE );
			glColorMask ( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		}

		framebuffer.unbind ();
		// combine
		glDisable ( GL_DEPTH_TEST );
		glDisable ( GL_BLEND );
		progCombineOIT.useProgram ();
		glBindTextureUnit ( 0, framebuffer.getTextureColor ().getHandle () );
		glDrawArrays ( GL_TRIANGLES, 0, 6 );

		ImGuiIO& io = ImGui::GetIO ();
		io.DisplaySize = ImVec2 ( ( float ) width, ( float ) height );
		ImGui::NewFrame ();
		ImGui::Begin ( "Control", nullptr );
		ImGui::Text ( "Draw:" );
		ImGui::Checkbox ( "Opaque meshes", &g_DrawOpaque );
		ImGui::Checkbox ( "Transparent meshes", &g_DrawTransparent );
		ImGui::Checkbox ( "Grid", &g_DrawGrid );
		ImGui::End ();
		ImGui::Render ();
		rendererUI.render ( width, height, ImGui::GetDrawData () );

		app.swapBuffers ();
	}

	return 0;
}