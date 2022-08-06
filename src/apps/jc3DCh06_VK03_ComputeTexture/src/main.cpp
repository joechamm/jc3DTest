#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkRenderers/VulkanSingleQuad.h>
#include <jcVkFramework/vkRenderers/VulkanComputedImage.h>
#include <jcVkFramework/vkRenderers/VulkanClear.h>
#include <jcVkFramework/vkRenderers/VulkanFinish.h>

#include <jcCommonframework/ResourceString.h>

const uint32_t kScreenWidth = 1280;
const uint32_t kScreenHeight = 720;

GLFWwindow* window;

VulkanInstance vk;
VulkanRenderDevice vkDev;

std::unique_ptr<VulkanSingleQuadRenderer> quad;
std::unique_ptr<VulkanClear> clear;
std::unique_ptr<VulkanFinish> finish;

std::unique_ptr<ComputedImage> imgGen;

void composeFrame ( VkCommandBuffer commandBuffer, uint32_t imageIndex )
{
	clear->fillCommandBuffer ( commandBuffer, imageIndex );

	insertComputedImageBarrier ( commandBuffer, imgGen->computed_.image );

	quad->fillCommandBuffer ( commandBuffer, imageIndex );
	finish->fillCommandBuffer ( commandBuffer, imageIndex );
}

int main ()
{
	window = initVulkanApp ( kScreenWidth, kScreenHeight );

	glfwSetKeyCallback (
		window,
		[]( GLFWwindow* window, int key, int scancode, int action, int mods )
		{
			const bool pressed = action != GLFW_RELEASE;
			if ( key == GLFW_KEY_ESCAPE && pressed )
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
		} );

//	createInstance ( &vk.instance );
	createInstanceWithDebugging ( &vk.instance, "jc3DTest Chapter 06 Compute Texture Exercise" );

	if ( !setupDebugCallbacks ( vk.instance, &vk.messenger ) ||
		glfwCreateWindowSurface ( vk.instance, window, nullptr, &vk.surface ) ||
		!initVulkanRenderDeviceWithCompute ( vk, vkDev, kScreenWidth, kScreenHeight, VkPhysicalDeviceFeatures{} ) )
		exit ( EXIT_FAILURE );

	VulkanImage nullTexture = { .image = VK_NULL_HANDLE, .imageView = VK_NULL_HANDLE };

	string compShdFilename = appendToRoot ( "assets/shaders/VK06_compute_texture.comp" );

	clear = std::make_unique<VulkanClear> ( vkDev, nullTexture );
	finish = std::make_unique<VulkanFinish> ( vkDev, nullTexture );
	imgGen = std::make_unique<ComputedImage> ( vkDev, compShdFilename.c_str (), 1024, 1024, false );
	quad = std::make_unique<VulkanSingleQuadRenderer> ( vkDev, imgGen->computed_, imgGen->computedImageSampler_ );

	do
	{
		float thisTime = (float)glfwGetTime ();
		imgGen->fillComputeComandBuffer ( &thisTime, sizeof ( float ), imgGen->computedWidth_ / 16, imgGen->computedHeight_ / 16, 1 );
		imgGen->submit ();
		vkDeviceWaitIdle ( vkDev.device );

		drawFrame ( vkDev, []( uint32_t ) {}, composeFrame );

		glfwPollEvents ();

		vkDeviceWaitIdle ( vkDev.device );
	} while ( !glfwWindowShouldClose ( window ) );

	imgGen->waitFence ();

	destroyVulkanImage ( vkDev.device, imgGen->computed_ );
	vkDestroySampler ( vkDev.device, imgGen->computedImageSampler_, nullptr );

	quad = nullptr;
	clear = nullptr;
	finish = nullptr;
	imgGen = nullptr;

	destroyVulkanRenderDevice ( vkDev );
	destroyVulkanInstance ( vk );

	glfwTerminate ();
	glslang_finalize_process ();

	return 0;
}