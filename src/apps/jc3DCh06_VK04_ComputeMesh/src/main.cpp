#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
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
#include "vkRenderers/VulkanClear.h"
#include "vkRenderers/VulkanFinish.h"
#include "vkRenderers/VulkanMultiMeshRenderer.h"
#include "EasyProfilerWrapper.h"
#include "Camera.h"

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

std::unique_ptr<MultiMeshRenderer> multiRenderer;
std::unique_ptr<VulkanClear> clear;
std::unique_ptr<VulkanFinish> finish;

FramesPerSecondCounter fpsCounter ( 0.02f );

struct MouseState
{
	glm::vec2 pos = glm::vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

CameraPositioner_FirstPerson positioner_firstPerson ( vec3(0.0f, -5.0f, 15.0f), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
Camera camera = Camera ( positioner_firstPerson );

bool initVulkan ()
{
	EASY_FUNCTION ();

//	createInstance ( &vk.instance );
	createInstanceWithDebugging ( &vk.instance, "jc3DCh05_VK01_MultiMeshDraw" );

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

	if ( !initVulkanRenderDevice ( vk, vkDev, kScreenWidth, kScreenHeight, isDeviceSuitable, { .multiDrawIndirect = VK_TRUE, .drawIndirectFirstInstance = VK_TRUE } ) )
	{
		printf ( "Failed to initialize vulkan render device\n" );
		exit ( EXIT_FAILURE );
	}

	string meshesFilename = appendToRoot ( "assets/models/exterior/test.meshes" );
	string instancesFilename = appendToRoot ( "assets/models/exterior/test.meshes.drawData" );

	string vtxShaderFilename = appendToRoot ( "assets/shaders/VK05.vert" );
	string fragShaderFilename = appendToRoot ( "assets/shaders/VK05.frag" );

	printf ( "Creating VulkanClear\n" );
	clear = std::make_unique<VulkanClear> ( vkDev, VulkanImage() );
	printf ( "Creating VulkanFinish\n" );
	finish = std::make_unique<VulkanFinish> ( vkDev, VulkanImage() );
	printf ( "Creating MultiMeshRenderer\n" );
	multiRenderer = std::make_unique<MultiMeshRenderer> ( vkDev, 
		meshesFilename.c_str (),
		instancesFilename.c_str (), 
		"", 
		vtxShaderFilename.c_str (), fragShaderFilename.c_str () );

	return true;
}

void terminateVulkan ()
{
	
	finish = nullptr;
	clear = nullptr;
	multiRenderer = nullptr;

	destroyVulkanRenderDevice ( vkDev );
	destroyVulkanInstance ( vk );
}

void update3D ( uint32_t imageIndex )
{
	int width, height;
	glfwGetFramebufferSize ( window, &width, &height );
	const float ratio = width / (float)height;

	const mat4 m1 = glm::rotate ( mat4 ( 1.0f ), glm::pi<float> (), vec3 ( 1, 0, 0 ) );
	const mat4 p = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );

	const mat4 view = camera.getViewMatrix ();
	const mat4 mtx = p * view * m1;

	{
		EASY_BLOCK ( "UpdateUniformBuffers" );

		multiRenderer->updateUniformBuffer ( vkDev, imageIndex, mtx );
		
		EASY_END_BLOCK;
	}
}

void composeFrame ( uint32_t imageIndex, const std::vector<RendererBase*>& renderers )
{
	update3D ( imageIndex );
	
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

	window = glfwCreateWindow ( kScreenWidth, kScreenHeight, "Vulkan Multi Mesh Draw App", nullptr, nullptr );

	if ( !window )
	{
		glfwTerminate ();
		exit ( EXIT_FAILURE );
	}

	glfwSetCursorPosCallback (
		window,
		[]( auto* window, double x, double y ) {
			if ( mouseState.pressedLeft )
			{
				mouseState.pos.x = static_cast<float>(x);
				mouseState.pos.y = static_cast<float>(y);
			}
		}
	);

	glfwSetMouseButtonCallback (
		window,
		[]( auto* window, int button, int action, int mods ) {
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

	double timeStamp = glfwGetTime ();
	float deltaSeconds = 0.0f;

	const std::vector<RendererBase*> renderers = { clear.get (), multiRenderer.get(), finish.get () };

	while ( !glfwWindowShouldClose ( window ) )
	{
		{
			EASY_BLOCK ( "UpdateCameraPositioners" );
			positioner_firstPerson.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft );
			EASY_END_BLOCK;
		}

		const double newTimeStamp = glfwGetTime ();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		const bool frameRendered = drawFrame ( renderers );		

		{
			EASY_BLOCK ( "PollEvents" );
			glfwPollEvents ();
			EASY_END_BLOCK;
		}

	}

	terminateVulkan ();
	glfwTerminate ();
	glslang_finalize_process ();

	PROFILER_DUMP ( "profiling.prof" );

	return 0;
}