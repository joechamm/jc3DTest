#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <deque>
#include <memory>
#include <limits>
#include <string>

#include "Utils.h"
#include "UtilsMath.h"
#include "UtilsFPS.h"
#include "UtilsVulkan.h"
#include "vkRenderers/VulkanCanvas.h"
#include "vkRenderers/VulkanCube.h"
#include "vkRenderers/VulkanImGui.h"
#include "vkRenderers/VulkanClear.h"
#include "vkRenderers/VulkanFinish.h"
#include "vkRenderers/VulkanModelRenderer.h"
#include "EasyProfilerWrapper.h"
#include "Camera.h"
#include "Graph.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <string>

#include <helpers/RootDir.h>
#include "ResourceString.h"

using glm::mat4;
using glm::vec3;
using glm::vec4;
using std::string;

const uint32_t kScreenWidth = 1280;
const uint32_t kScreenHeight = 720;

GLFWwindow* window;

VulkanInstance vk;
VulkanRenderDevice vkDev;

std::unique_ptr<ImGuiRenderer> imgui;
std::unique_ptr<ModelRenderer> modelRenderer;
std::unique_ptr<CubeRenderer> cubeRenderer;
std::unique_ptr<VulkanCanvas> canvas;
std::unique_ptr<VulkanCanvas> canvas2d;
std::unique_ptr<VulkanClear> clear;
std::unique_ptr<VulkanFinish> finish;

FramesPerSecondCounter fpsCounter ( 0.02f );
LinearGraph fpsGraph;
LinearGraph sineGraph ( 4096 );

struct MouseState
{
	glm::vec2 pos = glm::vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

glm::vec3 cameraPos ( 0.0f, 0.0f, 0.0f );
glm::vec3 cameraAngles ( -45.0f, 0.0f, 0.0f );

CameraPositioner_FirstPerson positioner_firstPerson ( cameraPos, vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
CameraPositioner_MoveTo positioner_moveTo ( cameraPos, cameraAngles );
Camera camera = Camera ( positioner_firstPerson );

// ImGUI stuff
const char* cameraType = "FirstPerson";
const char* comboBoxItems[] = { "FirstPerson", "MoveTo" };
const char* currentComboBoxItem = cameraType;

bool initVulkan ()
{
	EASY_FUNCTION ();

//	createInstance ( &vk.instance );
	createInstanceWithDebugging ( &vk.instance, "jc3DCh04_VK01_DemoApp" );

	if ( !setupDebugCallbacks ( vk.instance, &vk.messenger ) )
	{
		printf ( "Failed to setup debug callbacks\n" );
		exit ( EXIT_FAILURE );
	}

	if ( glfwCreateWindowSurface ( vk.instance, window, nullptr, &vk.surface ) != VK_SUCCESS )
	{
		printf ( "Failed to create window surface\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !initVulkanRenderDevice ( vk, vkDev, kScreenWidth, kScreenHeight, isDeviceSuitable, { .geometryShader = VK_TRUE } ) )
	{
		printf ( "Failed to initialize vulkan render device\n" );
		exit ( EXIT_FAILURE );
	}

	string modelFilename = appendToRoot ( "assets/models/rubber_duck/scene.gltf" );
	string imgFilename = appendToRoot ( "assets/images/sampleSTB.jpg" );
	string envFilename = appendToRoot ( "assets/images/piazza_bologni_1k.hdr" );

	printf ( "Creating ImGuiRenderer\n" );
	imgui = std::make_unique<ImGuiRenderer> ( vkDev );
	printf ( "Creating ModelRenderer\n" );
	modelRenderer = std::make_unique<ModelRenderer> ( vkDev, modelFilename.c_str (), imgFilename.c_str (), (uint32_t)sizeof ( glm::mat4 ) );
	printf ( "Creating CubeRenderer\n" );
	cubeRenderer = std::make_unique<CubeRenderer> ( vkDev, modelRenderer->getDepthTexture (), envFilename.c_str () );
	printf ( "Creating VulkanClear\n" );
	clear = std::make_unique<VulkanClear> ( vkDev, modelRenderer->getDepthTexture () );
	printf ( "Creating VulkanFinish\n" );
	finish = std::make_unique<VulkanFinish> ( vkDev, modelRenderer->getDepthTexture () );
	printf ( "Creating VulkanCanvas (canvas2d)\n" );
	canvas2d = std::make_unique<VulkanCanvas> ( vkDev, VulkanImage{ .image = VK_NULL_HANDLE, .imageView = VK_NULL_HANDLE } );
	printf ( "Creating VulkanCanvas (canvas)\n" );
	canvas = std::make_unique<VulkanCanvas> ( vkDev, modelRenderer->getDepthTexture () );

	return true;
}

void terminateVulkan ()
{
	canvas = nullptr;
	canvas2d = nullptr;
	finish = nullptr;
	clear = nullptr;
	cubeRenderer = nullptr;
	modelRenderer = nullptr;
	imgui = nullptr;
	destroyVulkanRenderDevice ( vkDev );
	destroyVulkanInstance ( vk );
}

void reinitCamera ()
{
	if ( !strcmp ( cameraType, "FirstPerson" ) )
	{
		camera = Camera ( positioner_firstPerson );
	}
	else
	{
		if ( !strcmp ( cameraType, "MoveTo" ) )
		{
			positioner_moveTo.setDesiredPosition ( cameraPos );
			positioner_moveTo.setDesiredAngles ( cameraAngles.x, cameraAngles.y, cameraAngles.z );
			camera = Camera ( positioner_moveTo );
		}
	}
}

void renderGUI ( uint32_t imageIndex )
{
	EASY_FUNCTION ();

	int width, height;
	glfwGetFramebufferSize ( window, &width, &height );

	ImGuiIO& io = ImGui::GetIO ();
	io.DisplaySize = ImVec2 ( (float)width, (float)height );
	ImGui::NewFrame ();

	const ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoBackground;

	ImGui::SetNextWindowPos ( ImVec2 ( 0, 0 ) );
	ImGui::Begin ( "Statistics", nullptr, flags );
	ImGui::Text ( "FPS: %.2f", fpsCounter.getFPS () );
	ImGui::End ();

	ImGui::Begin ( "Camera Control", nullptr );
	{
		if ( ImGui::BeginCombo ( "##combo", currentComboBoxItem ) ) // The second parameter is the label previewd before opening the combo.
		{
			for ( int n = 0; n < IM_ARRAYSIZE ( comboBoxItems ); n++ )
			{
				const bool isSelected = (currentComboBoxItem == comboBoxItems[n]);

				if ( ImGui::Selectable ( comboBoxItems[n], isSelected ) )
				{
					currentComboBoxItem = comboBoxItems[n];
				}

				if ( isSelected )
				{
					ImGui::SetItemDefaultFocus (); // You may set the initial focus when opening teh combo (scrolling + for keyboard navigation support)
				}
			}
			ImGui::EndCombo ();
		}

		if ( !strcmp ( cameraType, "MoveTo" ) )
		{
			if ( ImGui::SliderFloat3 ( "Position", glm::value_ptr ( cameraPos ), -10.0f, 10.0f ) )
			{
				positioner_moveTo.setDesiredPosition ( cameraPos );
			}
			if ( ImGui::SliderFloat3 ( "Pitch/Pan/Roll", glm::value_ptr ( cameraAngles ), -90.0f, 90.0f ) )
			{
				positioner_moveTo.setDesiredAngles ( cameraAngles );
			}
		}

		if ( currentComboBoxItem && strcmp ( currentComboBoxItem, cameraType ) )
		{
			printf ( "Selected new camera type: %s]n", currentComboBoxItem );
			cameraType = currentComboBoxItem;
			reinitCamera ();
		}
	}

	ImGui::End ();
	ImGui::Render ();

	imgui->updateBuffers ( vkDev, imageIndex, ImGui::GetDrawData () );
}

void update3D ( uint32_t imageIndex )
{
	int width, height;
	glfwGetFramebufferSize ( window, &width, &height );
	const float ratio = width / (float)height;

	const mat4 m1 = glm::rotate ( glm::translate ( mat4 ( 1.0f ), vec3 ( 0.0f, 0.5f, -1.5f ) ) * glm::rotate ( mat4 ( 1.0f ), glm::pi<float> (), vec3 ( 1, 0, 0 ) ), (float)glfwGetTime (), vec3 ( 0.0f, 1.0f, 0.0f ) );
	const mat4 p = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );

	const mat4 view = camera.getViewMatrix ();
	const mat4 mtx = p * view * m1;

	{
		EASY_BLOCK ( "UpdateUniformBuffers" );

		modelRenderer->updateUniformBuffer ( vkDev, imageIndex, glm::value_ptr ( mtx ), sizeof ( mat4 ) );
		canvas->updateUniformBuffer ( vkDev, p * view, 0.0f, imageIndex );
		canvas2d->updateUniformBuffer ( vkDev, glm::ortho ( 0, 1, 1, 0 ), 0.0f, imageIndex );
		cubeRenderer->updateUniformBuffer ( vkDev, imageIndex, p * view * m1 );
		
		EASY_END_BLOCK;
	}
}

void update2D ( uint32_t imageIndex )
{
	canvas2d->clear ();
	sineGraph.renderGraph ( *canvas2d.get (), vec4 ( 0.0f, 1.0f, 0.0f, 1.0f ) );
	fpsGraph.renderGraph ( *canvas2d.get () );
	canvas2d->updateBuffer ( vkDev, imageIndex );
}

void composeFrame ( uint32_t imageIndex, const std::vector<RendererBase*>& renderers )
{
	update3D ( imageIndex );
	renderGUI ( imageIndex );
	update2D ( imageIndex );

	EASY_BLOCK ( "FillCommandBuffers" );

	VkCommandBuffer commandBuffer = vkDev.commandBuffers[imageIndex];

	const VkCommandBufferBeginInfo bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK ( vkBeginCommandBuffer ( commandBuffer, &bi ) );

	for ( auto& r : renderers )
	{
		r->fillCommandBuffer ( commandBuffer, imageIndex );
	}

	VK_CHECK ( vkEndCommandBuffer ( commandBuffer ) );

	EASY_END_BLOCK;
}

bool drawFrame ( const std::vector<RendererBase*>& renderers )
{
	EASY_FUNCTION ();

	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR ( vkDev.device, vkDev.swapchain, 0, vkDev.semaphore, VK_NULL_HANDLE, &imageIndex );
	VK_CHECK ( vkResetCommandPool ( vkDev.device, vkDev.commandPool, 0 ) );

	if ( result != VK_SUCCESS )
	{
		return false;
	}

	composeFrame ( imageIndex, renderers );

	const VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };  // or even VERTEX_SHADER_STAGE

	const VkSubmitInfo si = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.semaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vkDev.commandBuffers[imageIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vkDev.renderSemaphore
	};

	{
		EASY_BLOCK ( "vkQueueSubmit", profiler::colors::Magenta );
		VK_CHECK ( vkQueueSubmit ( vkDev.graphicsQueue, 1, &si, nullptr ) );
		EASY_END_BLOCK;
	}

	const VkPresentInfoKHR pi = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &vkDev.swapchain,
		.pImageIndices = &imageIndex
	};

	{
		EASY_BLOCK ( "vkQueuePresentKHR", profiler::colors::Magenta );
		VK_CHECK ( vkQueuePresentKHR ( vkDev.graphicsQueue, &pi ) );
		EASY_END_BLOCK;
	}

	{
		EASY_BLOCK ( "vkDeviceWaitIdle", profiler::colors::Red );
		VK_CHECK ( vkDeviceWaitIdle ( vkDev.device ) );
		EASY_END_BLOCK;
	}

	return true;
}


int main ()
{
	EASY_PROFILER_ENABLE;
	EASY_MAIN_THREAD;

	// initialize the glslang compiler, the Volk library, and GLFW
	glslang_initialize_process ();
	volkInitialize ();
	if ( !glfwInit () )
	{
		exit ( EXIT_FAILURE );
	}
	if ( !glfwVulkanSupported () )
	{
		exit ( EXIT_FAILURE );
	}
	// disable OpenGL context creation
	glfwWindowHint ( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint ( GLFW_RESIZABLE, GL_FALSE );

	ImGui::CreateContext ();
	ImGuiIO& io = ImGui::GetIO ();

	window = glfwCreateWindow ( kScreenWidth, kScreenHeight, "Vulkan Demo App", nullptr, nullptr );

	if ( !window )
	{
		glfwTerminate ();
		exit ( EXIT_FAILURE );
	}

	glfwSetCursorPosCallback (
		window,
		[]( auto* window, double x, double y ) {
			ImGui::GetIO ().MousePos = ImVec2 ( (float)x, (float)y );
		}
	);

	glfwSetMouseButtonCallback (
		window,
		[]( auto* window, int button, int action, int mods ) {
			auto& io = ImGui::GetIO ();
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			io.MouseDown[idx] = action == GLFW_PRESS;

			if ( button == GLFW_MOUSE_BUTTON_LEFT )
			{
				mouseState.pressedLeft = action == GLFW_PRESS;
			}
		}
	);


	glfwSetKeyCallback (
		window,
		[]( GLFWwindow* window, int key, int scancode, int action, int mods )
		{
			const bool pressed = action != GLFW_RELEASE;
			if ( key == GLFW_KEY_ESCAPE && pressed )
			{
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
			}
			if ( key == GLFW_KEY_W )
			{
				positioner_firstPerson.movement_.forward_ = pressed;
			}
			if ( key == GLFW_KEY_S )
			{
				positioner_firstPerson.movement_.backward_ = pressed;
			}
			if ( key == GLFW_KEY_A )
			{
				positioner_firstPerson.movement_.left_ = pressed;
			}
			if ( key == GLFW_KEY_D )
			{
				positioner_firstPerson.movement_.right_ = pressed;
			}
			if ( key == GLFW_KEY_SPACE )
			{
				positioner_firstPerson.setUpVector ( vec3 ( 0.0f, 1.0f, 0.0f ) );
			}
		}
	);

	initVulkan ();

	{
		canvas->plane3d ( vec3 ( 0.0f, 1.5f, 0.0f ), vec3 ( 1.0f, 0.0f, 0.0f ), vec3 ( 0.0f, 0.0f, 1.0f ), 40, 40, 10.0f, 10.0f, vec4 ( 1.0f, 0.0f, 0.0f, 1.0f ), vec4 ( 0.0f, 1.0f, 0.0f, 1.0f ) );

		for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
		{
			canvas->updateBuffer ( vkDev, i );
		}
	}

	double timeStamp = glfwGetTime ();
	float deltaSeconds = 0.0f;

	const std::vector<RendererBase*> renderers = { clear.get (), cubeRenderer.get (), modelRenderer.get (), canvas.get (), canvas2d.get (), imgui.get (), finish.get () };
//	const std::vector<RendererBase*> renderers = { clear.get (), modelRenderer.get (), canvas.get (), canvas2d.get (), imgui.get (), finish.get () };

	while ( !glfwWindowShouldClose ( window ) )
	{
		{
			EASY_BLOCK ( "UpdateCameraPositioners" );
			positioner_firstPerson.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft );
			positioner_moveTo.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft );
			EASY_END_BLOCK;
		}

		const double newTimeStamp = glfwGetTime ();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		const bool frameRendered = drawFrame ( renderers );

		if ( fpsCounter.tick ( deltaSeconds, frameRendered ))
		{
			fpsGraph.addPoint(fpsCounter.getFPS());
		}

		sineGraph.addPoint ( (float)sin ( glfwGetTime () * 10.0 ) );

		{
			EASY_BLOCK ( "PollEvents" );
			glfwPollEvents ();
			EASY_END_BLOCK;
		}

	}

	ImGui::DestroyContext ();

	terminateVulkan ();
	glfwTerminate ();
	glslang_finalize_process ();

	PROFILER_DUMP ( "profiling.prof" );

	return 0;
}