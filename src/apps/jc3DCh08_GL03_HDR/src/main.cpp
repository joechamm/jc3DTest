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

struct HDRParams
{
	float exposure_ = 0.9f;
	float maxWhite_ = 1.17f;
	float bloomStrength_ = 1.1f;
} g_HDRParams;

static_assert(sizeof ( HDRParams ) <= sizeof ( PerFrameData ));

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

CameraPositioner_FirstPerson positioner ( vec3 ( -15.81f, 5.18f, -5.81f ), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
Camera camera ( positioner );
bool g_EnableHDR = true;

int main ()
{
	GLFWApp app;

	GLShader shdGridVertex ( appendToRoot ( "assets/shaders/GL08_grid.vert" ).c_str () );
	GLShader shdGridFragment ( appendToRoot ( "assets/shaders/GL08_grid.frag" ).c_str () );
	GLProgram progGrid ( shdGridVertex, shdGridFragment );

	GLShader shdFullScreenQuadVertex ( appendToRoot ( "assets/shaders/GL08_FullScreenQuad.vert" ).c_str () );

	GLShader shdCombineHDR ( appendToRoot ( "assets/shaders/GL08_HDR.frag" ).c_str () );
	GLProgram progCombineHDR ( shdFullScreenQuadVertex, shdCombineHDR );

	GLShader shdBlurX ( appendToRoot ( "assets/shaders/GL08_BlurX.frag" ).c_str () );
	GLShader shdBlurY ( appendToRoot ( "assets/shaders/GL08_BlurY.frag" ).c_str () );
	GLProgram progBlurX ( shdFullScreenQuadVertex, shdBlurX );
	GLProgram progBlurY ( shdFullScreenQuadVertex, shdBlurY );

	GLShader shdToLuminance ( appendToRoot ( "assets/shaders/GL08_ToLuminance.frag" ).c_str () );
	GLProgram progToLuminance ( shdFullScreenQuadVertex, shdToLuminance );

	GLShader shdBrightPass ( appendToRoot ( "assets/shaders/GL08_BrightPass.frag" ).c_str () );
	GLProgram progBrightPass ( shdFullScreenQuadVertex, shdBrightPass );
	
	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );

	GLBuffer perFrameDataBuffer ( kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle (), 0, kUniformBufferSize );

	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable ( GL_DEPTH_TEST );

	GLShader shaderVertex(appendToRoot("assets/shaders/GL08_scene_IBL.vert").c_str());
	GLShader shaderFragment(appendToRoot("assets/shaders/GL08_scene_IBL.frag").c_str());
	GLProgram program(shaderVertex, shaderFragment);

	GLSceneData sceneData1(appendToRoot("assets/meshes/test.meshes").c_str(), appendToRoot("assets/meshes/test.scene").c_str(), appendToRoot("assets/meshes/test.materials").c_str());
	GLSceneData sceneData2(appendToRoot("assets/meshes/test2.meshes").c_str(), appendToRoot("assets/meshes/test2.scene").c_str(), appendToRoot("assets/meshes/test2.materials").c_str());

	GLMesh mesh1(sceneData1);
	GLMesh mesh2(sceneData2);

	glfwSetCursorPosCallback(
		app.getWindow(),
		[](auto* window, double x, double y)
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			mouseState.pos.x = static_cast<float>(x / width);
			mouseState.pos.y = static_cast<float>(y / height);
			ImGui::GetIO().MousePos = ImVec2((float)x, (float)y);
		}
	);

	glfwSetMouseButtonCallback(
		app.getWindow(),
		[](auto* window, int button, int action, int mods)
		{
			auto& io = ImGui::GetIO();
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			io.MouseDown[idx] = action == GLFW_PRESS;

			if (!io.WantCaptureMouse)
				if (button == GLFW_MOUSE_BUTTON_LEFT)
					mouseState.pressedLeft = action == GLFW_PRESS;
		}
	);

	glfwSetKeyCallback(
		app.getWindow(),
		[](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			const bool pressed = action != GLFW_RELEASE;
			if (key == GLFW_KEY_ESCAPE && pressed)
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			if (key == GLFW_KEY_W)
				positioner.movement_.forward_ = pressed;
			if (key == GLFW_KEY_S)
				positioner.movement_.backward_ = pressed;
			if (key == GLFW_KEY_A)
				positioner.movement_.left_ = pressed;
			if (key == GLFW_KEY_D)
				positioner.movement_.right_ = pressed;
			if (key == GLFW_KEY_1)
				positioner.movement_.up_ = pressed;
			if (key == GLFW_KEY_2)
				positioner.movement_.down_ = pressed;
			if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
				positioner.movement_.fastSpeed_ = pressed;
			if (key == GLFW_KEY_SPACE)
				positioner.setUpVector(vec3(0.0f, 1.0f, 0.0f));
		}
	);

	positioner.maxSpeed_ = 1.0f;

	double timeStamp = glfwGetTime();
	float deltaSeconds = 0.0f;

	// offscreen render targets
	int width, height;
	glfwGetFramebufferSize(app.getWindow(), &width, &height);
	GLFramebuffer framebuffer(width, height, GL_RGBA16F, GL_DEPTH_COMPONENT24);
	GLFramebuffer luminance(64, 64, GL_R16F, 0);
	GLFramebuffer brightPass(256, 256, GL_RGBA16F, 0);
	GLFramebuffer bloom1(256, 256, GL_RGBA16F, 0);
	GLFramebuffer bloom2(256, 256, GL_RGBA16F, 0);
	// create texture view into the last mip-level (1x1 pixel) of our luminance framebuffer
	GLuint luminance1x1;
	glGenTextures(1, &luminance1x1);
	glTextureView(luminance1x1, GL_TEXTURE_2D, luminance.getTextureColor().getHandle(), GL_R16F, 6, 1, 0, 1);
	const GLint Mask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
	glTextureParameteriv(luminance1x1, GL_TEXTURE_SWIZZLE_RGBA, Mask);

	// cube map
	GLTexture envMap(GL_TEXTURE_CUBE_MAP, appendToRoot("assets/images/immenstadter_horn_2k.hdr").c_str());
	GLTexture envMapIrradiance(GL_TEXTURE_CUBE_MAP, appendToRoot("assets/images/immenstadter_horn_2k_irradiance.hdr").c_str());

	GLShader shdCubeVertex(appendToRoot("assets/shaders/GL08_cube.vert").c_str());
	GLShader shdCubeFragment(appendToRoot("assets/shaders/GL08_cube.frag").c_str());
	GLProgram progCube(shdCubeVertex, shdCubeFragment);

	GLuint dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);
	const GLuint pbrTextures[] = { envMap.getHandle(), envMapIrradiance.getHandle() };
	glBindTextures(5, 2, pbrTextures);

	ImGuiGLRenderer rendererUI;

	while (!glfwWindowShouldClose(app.getWindow())) 
	{
		positioner.update(deltaSeconds, mouseState.pos, mouseState.pressedLeft);

		const double newTimeStamp = glfwGetTime();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		int width, height;
		glfwGetFramebufferSize(app.getWindow(), &width, &height);
		const float ratio = width / (float)height;

		glClearNamedFramebufferfv(framebuffer.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
		glClearNamedFramebufferfi(framebuffer.getHandle(), GL_DEPTH_STENCIL, 0, 1.0f, 0);

		const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
		const mat4 view = camera.getViewMatrix();

		const PerFrameData perFrameData = {
			.view = view,
			.proj = p,
			.cameraPos = vec4(camera.getPosition(), 1.0f)
		};

		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);

		// 1. Render scene
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		framebuffer.bind();
		// 1.0 Cube map
		progCube.useProgram();
		glBindTextureUnit(1, envMap.getHandle());
		glDepthMask(false);
		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthMask(true);
		// 1.1 Bistro
		program.useProgram();
		mesh1.draw(sceneData1);
		mesh2.draw(sceneData2);
		// 1.2 Grid
		glEnable(GL_BLEND);
		progGrid.useProgram();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
		framebuffer.unbind();

		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		// 2.1 Extract bright areas
		brightPass.bind();
		progBrightPass.useProgram();
		glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
		glDrawArrays(GL_TRIANGLES, 0, 6);
		brightPass.unbind();

		// 2.2 Downscale and convert to luminance
		luminance.bind();
		progToLuminance.useProgram();
		glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
		glDrawArrays(GL_TRIANGLES, 0, 6);
		luminance.unbind();
		glGenerateTextureMipmap(luminance.getTextureColor().getHandle());

		glBlitNamedFramebuffer(brightPass.getHandle(), bloom2.getHandle(), 0, 0, 256, 256, 0, 0, 256, 256, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		for (int i = 0; i != 4; i++) 
		{
			// 2.3 Blur X
			bloom1.bind();
			progBlurX.useProgram();
			glBindTextureUnit(0, bloom2.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
			bloom1.unbind();
			// 2.4 Blur Y
			bloom2.bind();
			progBlurY.useProgram();
			glBindTextureUnit(0, bloom1.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
			bloom2.unbind();
		}

		// 3. Apply tone mapping
		glViewport(0, 0, width, height);

		if (g_EnableHDR)
		{
			glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, sizeof(g_HDRParams), &g_HDRParams);
			progCombineHDR.useProgram();
			glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
			glBindTextureUnit(1, luminance1x1);
			glBindTextureUnit(2, bloom2.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		else 
		{
			glBlitNamedFramebuffer(framebuffer.getHandle(), 0, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();
		ImGui::Begin("Control", nullptr);
		ImGui::Checkbox("Enable HDR", &g_EnableHDR);
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !g_EnableHDR);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * g_EnableHDR ? 1.0f : 0.2f);
		ImGui::Separator();
		ImGui::Text("Average luminance:");
		ImGui::Image((void*)(intptr_t)luminance1x1, ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Separator();
		ImGui::SliderFloat("Exposure", &g_HDRParams.exposure_, 0.1f, 2.0f);
		ImGui::SliderFloat("Max White", &g_HDRParams.maxWhite_, 0.5f, 2.0f);
		ImGui::SliderFloat("Bloom strength", &g_HDRParams.bloomStrength_, 0.0f, 2.0f);
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
		ImGui::End();
		imguiTextureWindowGL("Color", framebuffer.getTextureColor().getHandle());
		imguiTextureWindowGL("Luminance", luminance.getTextureColor().getHandle());
		imguiTextureWindowGL("Bright Pass", brightPass.getTextureColor().getHandle());
		imguiTextureWindowGL("Bloom", bloom2.getTextureColor().getHandle());
		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		app.swapBuffers();
	}

	glDeleteTextures(1, &luminance1x1);

	return 0;
}