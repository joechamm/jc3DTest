#include "vkFramework/VulkanApp.h"
#include "vkRenderers/VulkanClear.h"
#include "vkRenderers/VulkanFinish.h"
#include "vkRenderers/VulkanQuadRenderer.h"

//#include "EasyProfilerWrapper.h"

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

	initVulkan ();

	printf ( "Textures loaded. Click to make an explosion.\n" );

	while ( !glfwWindowShouldClose ( window ) )
	{
		drawFrame ( vkDev, []( uint32_t ) {}, composeFrame );
		glfwPollEvents ();
	}

	terminateVulkan ();
	glfwTerminate ();
	glslang_finalize_process ();

//	PROFILER_DUMP ( "profiling.prof" );

	return 0;
}