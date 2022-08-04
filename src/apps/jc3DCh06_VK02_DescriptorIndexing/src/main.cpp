#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkRenderers/VulkanClear.h>
#include <jcVkFramework/vkRenderers/VulkanFinish.h>
#include <jcVkFramework/vkRenderers/VulkanQuadRenderer.h>

#include <jcVkFramework/UtilsVulkan.h>

//#include "EasyProfilerWrapper.h"

#include <helpers/RootDir.h>
#include <jcVkFramework/ResourceString.h>

using glm::mat4;
using glm::vec3;
using glm::vec4;
using std::string;

const uint32_t kScreenWidth = 1280;
const uint32_t kScreenHeight = 720;

GLFWwindow* window;

VulkanInstance vk;
VulkanRenderDevice vkDev;

std::unique_ptr<VulkanQuadRenderer> quadRenderer;
std::unique_ptr<VulkanClear> clear;
std::unique_ptr<VulkanFinish> finish;

const double kAnimationFPS = 60.0;
const uint32_t kNumFlipbookFrames = 100;

struct AnimationState
{
	vec2 position = vec2 ( 0.0f );
	double startTime = 0.0;
	uint32_t textureIndex = 0;
	uint32_t flipbookOffset = 0;
};

std::vector<AnimationState> animations;
//
//bool drawFrameFromMain ( VulkanRenderDevice& vkDev, const std::function<void ( uint32_t )>& updateBuffersFunc, const std::function<void ( VkCommandBuffer, uint32_t )>& composeFrameFunc )
//{
//	/*
//	*	Before we can render anything on the screen, we should acquire a framebuffer image from the swapchain. When the next image in the swapchain is not ready to be rendered, we return false.
//	*	This is not a fatal error but an indication that no frame has been rendered yet. The calling code decides what to do with the result.
//	*	One example of such a reaction can be skipping the frames-per-second (FPS) counter update:
//	*/
//	uint32_t imageIndex = 0;
//	VkResult result = vkAcquireNextImageKHR (/*VkDevice*/ vkDev.device, /*VkSwapchain*/ vkDev.swapchain,/*uint64_t timeout*/ 0,/*VkSemaphore*/ vkDev.semaphore,/*VkFence*/ VK_NULL_HANDLE, /*uint32_t *pImageIndex*/ &imageIndex );
//	if ( result != VK_SUCCESS )  return false;
//
//	/* reset the global command pool to allow the filling of new command buffers */
//	VK_CHECK ( vkResetCommandPool ( vkDev.device, vkDev.commandPool, 0 ) );
//
//	/*
//	*	The updateBuffersFunc() callback is invoked to update all the internal buffers for different renderers.
//	*	Revisit the Organizing Vulkan frame rendering code recipe from Chapter 4, Adding User Interaction and Productivity Tools for a discussion of frame composition.
//	*	This can be done in a more effective way—for example, by using a dedicated transfer queue and without waiting for all the GPU transfers to complete.
//	*	For the purpose of this book, we deliberately choose code simplicity over performance. A command buffer, corresponding to the selected swapchain image, is acquired
//	*/
//
//	updateBuffersFunc ( imageIndex );
//	VkCommandBuffer commandBuffer = vkDev.commandBuffers[imageIndex];
//	const VkCommandBufferBeginInfo bi = {
//		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//		.pNext = nullptr,
//		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
//		.pInheritanceInfo = nullptr
//	};
//
//	VK_CHECK ( vkBeginCommandBuffer ( commandBuffer, &bi ) );
//
//	/*
//	*	After we have started recording to the command buffer, the composeFrameFunc() callback is invoked to write the command buffer's contents from different renderers.
//	*	There is a large potential for optimizations here because Vulkan provides a primary-secondary command buffer separation, which can be used to record secondary buffers from multiple central processing unit (CPU) threads.
//	*	Once all the renderers have contributed to the command buffer, we stop recording
//	*/
//	composeFrameFunc ( commandBuffer, imageIndex );
//	VK_CHECK ( vkEndCommandBuffer ( commandBuffer ) );
//
//	/* Next comes the submission of the recorded command buffer to a GPU graphics queue. */
//	const VkPipelineStageFlags waitStages[] = {
//		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
//	};
//
//	const VkSubmitInfo si = {
//		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
//		.pNext = nullptr,
//		.waitSemaphoreCount = 1,
//		.pWaitSemaphores = &vkDev.semaphore,
//		.pWaitDstStageMask = waitStages,
//		.commandBufferCount = 1,
//		.pCommandBuffers = &vkDev.commandBuffers[imageIndex],
//		.signalSemaphoreCount = 1,
//		.pSignalSemaphores = &vkDev.renderSemaphore
//	};
//
//	VK_CHECK ( vkQueueSubmit ( vkDev.graphicsQueue, 1, &si, nullptr ) );
//
//	/* After submitting the command buffer to the graphics queue, the swapchain is presented to the screen */
//	const VkPresentInfoKHR pi = {
//		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
//		.pNext = nullptr,
//		.waitSemaphoreCount = 1,
//		.pWaitSemaphores = &vkDev.renderSemaphore,
//		.swapchainCount = 1,
//		.pSwapchains = &vkDev.swapchain,
//		.pImageIndices = &imageIndex
//	};
//
//	VK_CHECK ( vkQueuePresentKHR ( vkDev.graphicsQueue, &pi ) );
//
//	/* The final call to vkDeviceWaitIdle() ensures we ahve no frame tearing */
//	VK_CHECK ( vkDeviceWaitIdle ( vkDev.device ) );
//
//	return true;
//}
//
//void nullUpdateBuffersFunc ( uint32_t i )
//{
//	return;
//}

void updateAnimations ()
{
	for ( size_t i = 0; i < animations.size (); )
	{
		animations[i].textureIndex = animations[i].flipbookOffset + (uint32_t)(kAnimationFPS * ((glfwGetTime () - animations[i].startTime)));
		if ( animations[i].textureIndex - animations[i].flipbookOffset > kNumFlipbookFrames )
			animations.erase ( animations.begin () + i );
		else
			i++;
	}
}

void fillQuadsBuffer ( VulkanRenderDevice& vkDev, VulkanQuadRenderer& quadRenderer, size_t currentImage )
{
	const float aspect = (float)vkDev.framebufferWidth / (float)vkDev.framebufferHeight;
	const float quadSize = 0.5f;

	quadRenderer.clear ();
	quadRenderer.quad ( -quadSize, -quadSize * aspect, quadSize, quadSize * aspect );
	quadRenderer.updateBuffer ( vkDev, currentImage );
}

bool initVulkan ()
{
//	EASY_FUNCTION ();

//	createInstance ( &vk.instance );
	createInstanceWithDebugging ( &vk.instance, "jc3DCh06_VK02_DescriptorIndexing" );

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

	/* After the window surface is created, we construct an instance of the VkPhysicalDeviceDescriptorIndexingFeaturesEXT structure to enable non-uniform image array indexing and a variable descriptor set count */
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE
	};

	/* The required feature should be enabled using VkPhysicalDeviceFeatures */
	const VkPhysicalDeviceFeatures deviceFeatures = {
		.shaderSampledImageArrayDynamicIndexing = VK_TRUE
	};

	/* After that, both structures can be used to construct VkPhysicalDeviceFeatures2 */
	const VkPhysicalDeviceFeatures2 deviceFeatures2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &physicalDeviceDescriptorIndexingFeatures,
		.features = deviceFeatures
	};

	if ( !initVulkanRenderDevice2 ( vk, vkDev, kScreenWidth, kScreenHeight, isDeviceSuitable, deviceFeatures2) )
	{
		printf ( "Failed to initialize vulkan render device\n" );
		exit ( EXIT_FAILURE );
	}

	std::vector<std::string> textureFiles;
	for ( uint32_t j = 0; j < 3; j++ )
	{
		for ( uint32_t i = 0; i != kNumFlipbookFrames; i++ )
		{
			char fname[1024];
			snprintf ( fname, sizeof ( fname ), "assets/images/explosion%01u/explosion%02u-frame%03u.tga", j, j, i + 1 );		
			textureFiles.push_back ( appendToRoot ( fname ) );
		}
	}

	quadRenderer = std::make_unique<VulkanQuadRenderer> ( vkDev, textureFiles );

	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		fillQuadsBuffer ( vkDev, *quadRenderer.get (), i );
	}

	VulkanImage nullTexture = {
		.image = VK_NULL_HANDLE,
		.imageView = VK_NULL_HANDLE
	};

	clear = std::make_unique<VulkanClear> ( vkDev, nullTexture );
	finish = std::make_unique<VulkanFinish> ( vkDev, nullTexture );

	return VK_SUCCESS;
}

void terminateVulkan ()
{
	
	finish = nullptr;
	clear = nullptr;
	quadRenderer = nullptr;

	destroyVulkanRenderDevice ( vkDev );
	destroyVulkanInstance ( vk );
}

float mouseX = 0;
float mouseY = 0;

void composeFrame (VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
//	EASY_BLOCK ( "FillCommandBuffers" );

	clear->fillCommandBuffer ( commandBuffer, imageIndex );

	for ( size_t i = 0; i < animations.size (); i++ )
	{
		quadRenderer->pushConstants ( commandBuffer, animations[i].textureIndex, animations[i].position );
		quadRenderer->fillCommandBuffer ( commandBuffer, imageIndex );
	}

	finish->fillCommandBuffer ( commandBuffer, imageIndex );

//	EASY_END_BLOCK;
}

int main ()
{
//	EASY_PROFILER_ENABLE;
//	EASY_MAIN_THREAD;

	printf ( "initVulkanApp\n" );
	window = initVulkanApp ( kScreenWidth, kScreenHeight );

	glfwSetCursorPosCallback ( window, []( GLFWwindow* window, double xpos, double ypos ) { mouseX = (float)xpos; mouseY = (float)ypos; } );
	glfwSetMouseButtonCallback (
		window,
		[]( GLFWwindow* window, int button, int action, int mods )
		{
			if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS )
			{
				const float mx = (mouseX / (float)vkDev.framebufferWidth) * 2.0f - 1.0f;
				const float my = (mouseY / (float)vkDev.framebufferHeight) * 2.0f - 1.0f;

				animations.push_back ( AnimationState{
					.position = vec2 ( mx, my ),
					.startTime = glfwGetTime (),
					.textureIndex = 0,
					.flipbookOffset = kNumFlipbookFrames * (uint32_t)(rand () % 3)
					} );
			}
		} );

	glfwSetKeyCallback (
		window,
		[]( GLFWwindow* window, int key, int scancode, int action, int mods )
		{
			if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
			{
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
			}
		}
	);

	printf ( "initVulkan\n" );
	initVulkan ();

	printf ( "Textures loaded. Click to make an explosion.\n" );

//	const std::function<void ( uint32_t )>& tempUpdateBuffersFunc = nullUpdateBuffersFunc;
//	const std::function<void ( VkCommandBuffer, uint32_t )>& tempComposeFrameFunc = composeFrame;

//	std::function<void(uint32_t)

//		const std::function<void ( uint32_t )>& updateBuffersFunc, const std::function<void ( VkCommandBuffer, uint32_t )>& composeFrameFunc

	while ( !glfwWindowShouldClose ( window ) )
	{
//		drawVulkanAppFrame ( vkDev, nullUpdateBuffersFunc, composeFrame );
//		drawFrame ( vkDev, &nullUpdateBuffersFunc, composeFrame );
//		drawVulkanAppFrame ( vkDev, []( uint32_t ) {}, composeFrame );
//		drawFrame ( vkDev, []( uint32_t ) {}, composeFrame );
//		drawFrameFromMain ( vkDev, []( uint32_t ) {}, composeFrame );
//		drawVulkanAppFrame ( vkDev, []( uint32_t ) {}, composeFrame );
//		jcvkfw::jcDrawFrame ( vkDev, []( uint32_t ) {}, composeFrame );
		drawFrame(vkDev, []( uint32_t ) {}, composeFrame );
		glfwPollEvents ();
	}

	terminateVulkan ();
	glfwTerminate ();
	glslang_finalize_process ();

//	PROFILER_DUMP ( "profiling.prof" );

	return 0;
}