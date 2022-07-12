#include "Utils.h"
#include "UtilsVulkan.h"

#include <glslang/Include/ResourceLimits.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <cstdio>
#include <cstdlib>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;


static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback ( 
	VkDebugUtilsMessageSeverityFlagBitsEXT Severity, 
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, 
	void* UserData )
{
	printf ( "Validation layer: %s\n", CallbackData->pMessage );
	return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugReportCallback (
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* UserData )
{
	if ( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT )
		return VK_FALSE;
	printf ( "Debug callback (%s): %s\n", pLayerPrefix, pMessage );
	return VK_FALSE;
}

bool setupDebugCallbacks ( 
	VkInstance instance, 
	VkDebugUtilsMessengerEXT* messenger, 
	VkDebugReportCallbackEXT* reportCallback )
{
	const VkDebugUtilsMessengerCreateInfoEXT ci1 = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &vulkanDebugCallback,
		.pUserData = nullptr
	};
	VK_CHECK ( vkCreateDebugUtilsMessengerEXT ( instance, &ci1, nullptr, messenger ) );
	const VkDebugReportCallbackCreateInfoEXT ci2 = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		.pNext = nullptr,
		.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
		.pfnCallback = &vulkanDebugReportCallback,
		.pUserData = nullptr
	};
	VK_CHECK ( vkCreateDebugReportCallbackEXT ( instance, &ci2, nullptr, reportCallback ) );
	return true;

}

glslang_stage_t glslangShaderStageFromFileName ( const char* fileName )
{
	if ( endsWith ( fileName, ".vert" ) )
		return GLSLANG_STAGE_VERTEX;
	if ( endsWith ( fileName, ".frag" ) )
		return GLSLANG_STAGE_FRAGMENT;
	if ( endsWith ( fileName, ".geom" ) )
		return GLSLANG_STAGE_GEOMETRY;
	if ( endsWith ( fileName, ".comp" ) )
		return GLSLANG_STAGE_COMPUTE;
	if ( endsWith ( fileName, ".tesc" ) )
		return GLSLANG_STAGE_TESSCONTROL;
	if ( endsWith ( fileName, ".tese" ) )
		return GLSLANG_STAGE_TESSEVALUATION;

	return GLSLANG_STAGE_VERTEX;
}

namespace glslang
{
	static TBuiltInResource InitResources ()
	{
		TBuiltInResource Resources;

		Resources.maxLights = 32;
		Resources.maxClipPlanes = 6;
		Resources.maxTextureUnits = 32;
		Resources.maxTextureCoords = 32;
		Resources.maxVertexAttribs = 64;
		Resources.maxVertexUniformComponents = 4096;
		Resources.maxVaryingFloats = 64;
		Resources.maxVertexTextureImageUnits = 32;
		Resources.maxCombinedTextureImageUnits = 80;
		Resources.maxTextureImageUnits = 32;
		Resources.maxFragmentUniformComponents = 4096;
		Resources.maxDrawBuffers = 32;
		Resources.maxVertexUniformVectors = 128;
		Resources.maxVaryingVectors = 8;
		Resources.maxFragmentUniformVectors = 16;
		Resources.maxVertexOutputVectors = 16;
		Resources.maxFragmentInputVectors = 15;
		Resources.minProgramTexelOffset = -8;
		Resources.maxProgramTexelOffset = 7;
		Resources.maxClipDistances = 8;
		Resources.maxComputeWorkGroupCountX = 65535;
		Resources.maxComputeWorkGroupCountY = 65535;
		Resources.maxComputeWorkGroupCountZ = 65535;
		Resources.maxComputeWorkGroupSizeX = 1024;
		Resources.maxComputeWorkGroupSizeY = 1024;
		Resources.maxComputeWorkGroupSizeZ = 64;
		Resources.maxComputeUniformComponents = 1024;
		Resources.maxComputeTextureImageUnits = 16;
		Resources.maxComputeImageUniforms = 8;
		Resources.maxComputeAtomicCounters = 8;
		Resources.maxComputeAtomicCounterBuffers = 1;
		Resources.maxVaryingComponents = 60;
		Resources.maxVertexOutputComponents = 64;
		Resources.maxGeometryInputComponents = 64;
		Resources.maxGeometryOutputComponents = 128;
		Resources.maxFragmentInputComponents = 128;
		Resources.maxImageUnits = 8;
		Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
		Resources.maxCombinedShaderOutputResources = 8;
		Resources.maxImageSamples = 0;
		Resources.maxVertexImageUniforms = 0;
		Resources.maxTessControlImageUniforms = 0;
		Resources.maxTessEvaluationImageUniforms = 0;
		Resources.maxGeometryImageUniforms = 0;
		Resources.maxFragmentImageUniforms = 8;
		Resources.maxCombinedImageUniforms = 8;
		Resources.maxGeometryTextureImageUnits = 16;
		Resources.maxGeometryOutputVertices = 256;
		Resources.maxGeometryTotalOutputComponents = 1024;
		Resources.maxGeometryUniformComponents = 1024;
		Resources.maxGeometryVaryingComponents = 64;
		Resources.maxTessControlInputComponents = 128;
		Resources.maxTessControlOutputComponents = 128;
		Resources.maxTessControlTextureImageUnits = 16;
		Resources.maxTessControlUniformComponents = 1024;
		Resources.maxTessControlTotalOutputComponents = 4096;
		Resources.maxTessEvaluationInputComponents = 128;
		Resources.maxTessEvaluationOutputComponents = 128;
		Resources.maxTessEvaluationTextureImageUnits = 16;
		Resources.maxTessEvaluationUniformComponents = 1024;
		Resources.maxTessPatchComponents = 120;
		Resources.maxPatchVertices = 32;
		Resources.maxTessGenLevel = 64;
		Resources.maxViewports = 16;
		Resources.maxVertexAtomicCounters = 0;
		Resources.maxTessControlAtomicCounters = 0;
		Resources.maxTessEvaluationAtomicCounters = 0;
		Resources.maxGeometryAtomicCounters = 0;
		Resources.maxFragmentAtomicCounters = 8;
		Resources.maxCombinedAtomicCounters = 8;
		Resources.maxAtomicCounterBindings = 1;
		Resources.maxVertexAtomicCounterBuffers = 0;
		Resources.maxTessControlAtomicCounterBuffers = 0;
		Resources.maxTessEvaluationAtomicCounterBuffers = 0;
		Resources.maxGeometryAtomicCounterBuffers = 0;
		Resources.maxFragmentAtomicCounterBuffers = 1;
		Resources.maxCombinedAtomicCounterBuffers = 1;
		Resources.maxAtomicCounterBufferSize = 16384;
		Resources.maxTransformFeedbackBuffers = 4;
		Resources.maxTransformFeedbackInterleavedComponents = 64;
		Resources.maxCullDistances = 8;
		Resources.maxCombinedClipAndCullDistances = 8;
		Resources.maxSamples = 4;
		Resources.maxMeshOutputVerticesNV = 256;
		Resources.maxMeshOutputPrimitivesNV = 512;
		Resources.maxMeshWorkGroupSizeX_NV = 32;
		Resources.maxMeshWorkGroupSizeY_NV = 1;
		Resources.maxMeshWorkGroupSizeZ_NV = 1;
		Resources.maxTaskWorkGroupSizeX_NV = 32;
		Resources.maxTaskWorkGroupSizeY_NV = 1;
		Resources.maxTaskWorkGroupSizeZ_NV = 1;
		Resources.maxMeshViewCountNV = 4;

		Resources.limits.nonInductiveForLoops = 1;
		Resources.limits.whileLoops = 1;
		Resources.limits.doWhileLoops = 1;
		Resources.limits.generalUniformIndexing = 1;
		Resources.limits.generalAttributeMatrixVectorIndexing = 1;
		Resources.limits.generalVaryingIndexing = 1;
		Resources.limits.generalSamplerIndexing = 1;
		Resources.limits.generalVariableIndexing = 1;
		Resources.limits.generalConstantMatrixVectorIndexing = 1;

		return Resources;
	}
}

static_assert(sizeof ( TBuiltInResource ) == sizeof ( glslang_resource_t ));

static size_t compileShader ( glslang_stage_t stage, const char* shaderSource, ShaderModule& shaderModule )
{
	static TBuiltInResource DefaultTBuildInResources = glslang::InitResources ();

	const glslang_input_t input = {
		.language = GLSLANG_SOURCE_GLSL,
		.stage = stage,
		.client = GLSLANG_CLIENT_VULKAN,
		.client_version = GLSLANG_TARGET_VULKAN_1_1,
		.target_language = GLSLANG_TARGET_SPV,
		.target_language_version = GLSLANG_TARGET_SPV_1_3,
		.code = shaderSource,
		.default_version = 100,
		.default_profile = GLSLANG_NO_PROFILE,
		.force_default_version_and_profile = false,
		.forward_compatible = false,
		.messages = GLSLANG_MSG_DEFAULT_BIT,
		.resource = (const glslang_resource_t*)&DefaultTBuildInResources,
	};

	glslang_shader_t* shd = glslang_shader_create ( &input );

	if ( !glslang_shader_preprocess ( shd, &input ) )
	{
		fprintf ( stderr, "GLSL preprocessing failed\n" );
		fprintf ( stderr, "\n%s", glslang_shader_get_info_log ( shd ) );
		fprintf ( stderr, "\n%s", glslang_shader_get_info_debug_log ( shd ) );
		fprintf ( stderr, "code:\n%s", input.code );
		return 0;
	}

	if ( !glslang_shader_parse ( shd, &input ) )
	{
		fprintf ( stderr, "GLSL parsing failed\n" );
		fprintf ( stderr, "\n%s", glslang_shader_get_info_log ( shd ) );
		fprintf ( stderr, "\n%s", glslang_shader_get_info_debug_log ( shd ) );
		fprintf ( stderr, "%s", glslang_shader_get_preprocessed_code ( shd ) );
		return 0;
	}

	glslang_program_t* prog = glslang_program_create ();
	glslang_program_add_shader ( prog, shd );
	int msgs = GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT;
	if ( !glslang_program_link ( prog, msgs ) )
	{
		fprintf ( stderr, "GLSL linking failed\n" );
		fprintf ( stderr, "\n%s", glslang_program_get_info_log ( prog ) );
		fprintf ( stderr, "\n%s", glslang_program_get_info_debug_log ( prog ) );
		return 0;
	}

	glslang_program_SPIRV_generate ( prog, stage );
	shaderModule.SPIRV.resize ( glslang_program_SPIRV_get_size ( prog ) );
	glslang_program_SPIRV_get ( prog, shaderModule.SPIRV.data () );
	const char* spirv_messages = glslang_program_SPIRV_get_messages ( prog );
	if ( spirv_messages )
		fprintf ( stderr, "%s", spirv_messages );

	glslang_program_delete ( prog );
	glslang_shader_delete ( shd );
	return shaderModule.SPIRV.size ();
}

size_t compileShaderFile ( const char* file, ShaderModule& shaderModule )
{
	if ( auto shaderSource = readShaderFile ( file ); !shaderSource.empty () )
		return compileShader ( glslangShaderStageFromFileName ( file ), shaderSource.c_str (), shaderModule );

	return 0;		
}

void createInstance ( VkInstance* instance )
{
	// https://vulkan.lunarg.com/doc/view/1.1.108.0/windows/validation_layers.html
	const std::vector<const char*> layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> exts = {
		"VK_KHR_SURFACE",
#if defined(WIN32)
		"VK_KHR_win32_surface",
#endif
#if defined(__APPLE__)
		"VK_MVK_macos_surface",
#endif
#if defined(__linux__)
		"VK_KHR_xcb_surface",
#endif
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	};
	const VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Vulkan",
.applicationVersion = VK_MAKE_VERSION ( 1, 0, 0 ),
.pEngineName = "No Engine",
.engineVersion = VK_MAKE_VERSION ( 1, 0, 0 ),
.apiVersion = VK_API_VERSION_1_1
	};

	const VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(layers.size ()),
		.ppEnabledLayerNames = layers.data (),
		.enabledExtensionCount = static_cast<uint32_t>(exts.size ()),
		.ppEnabledExtensionNames = exts.data ()
	};
	VK_ASSERT ( vkCreateInstance ( &createInfo, nullptr, instance ) == VK_SUCCESS );

	volkLoadInstance ( *instance );
}

VkResult createDevice (
	VkPhysicalDevice physicalDevice,
	VkPhysicalDeviceFeatures deviceFeatures,
	uint32_t graphicsFamily,
	VkDevice* device )
{
	const std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	const float queuePriority = 1.0f;
	const VkDeviceQueueCreateInfo qci = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};

	const VkDeviceCreateInfo ci = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &qci,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size ()),
		.ppEnabledExtensionNames = extensions.data (),
		.pEnabledFeatures = &deviceFeatures
	};

	return vkCreateDevice ( physicalDevice, &ci, nullptr, device );
}

VkResult findSuitablePhysicalDevice (
	VkInstance instance,
	std::function<bool ( VkPhysicalDevice )> selector,
	VkPhysicalDevice* physicalDevice )
{
	uint32_t deviceCount = 0;
	VK_CHECK_RET ( vkEnumeratePhysicalDevices ( instance, &deviceCount, nullptr ) );
	if ( !deviceCount ) return VK_ERROR_INITIALIZATION_FAILED;
	std::vector<VkPhysicalDevice> devices ( deviceCount );
	VK_CHECK_RET ( vkEnumeratePhysicalDevices ( instance, &deviceCount, devices.data () ) );
	for ( const auto& device : devices )
	{
		if ( selector ( device ) )
		{
			*physicalDevice = device;
			return VK_SUCCESS;
		}
	}
	return VK_ERROR_INITIALIZATION_FAILED;
}

uint32_t findQueueFamilies (
	VkPhysicalDevice device,
	VkQueueFlags desiredFlags )
{
	uint32_t familyCount;
	vkGetPhysicalDeviceQueueFamilyProperties ( device, &familyCount, nullptr );
	std::vector<VkQueueFamilyProperties> families ( familyCount );
	vkGetPhysicalDeviceQueueFamilyProperties ( device, &familyCount, families.data () );
	for ( uint32_t i = 0; i != families.size (); i++ )
	{
		if ( families[i].queueCount && (families[i].queueFlags & desiredFlags) )
		{
			return i;
		}
	}
	return 0;
}

VkFormat findSupportedFormat ( 
	VkPhysicalDevice device, 
	const std::vector<VkFormat>& candidates, 
	VkImageTiling tiling, 
	VkFormatFeatureFlags features )
{
	const bool isLin = tiling == VK_IMAGE_TILING_LINEAR;
	const bool isOpt = tiling == VK_IMAGE_TILING_OPTIMAL;
	for ( VkFormat format : candidates )
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties ( device, format, &props );
		if ( isLin && (props.linearTilingFeatures & features) == features )
		{
			return format;
		} else if(isOpt && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	printf ( "failed to find supported format!\n" );
	exit ( 0 );
}

VkFormat findDepthFormat ( VkPhysicalDevice device )
{
	return findSupportedFormat ( device, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

bool hasStencilComponent ( VkFormat format )
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

uint32_t findMemoryType ( VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties ( device, &memProperties );
	for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
	{
		if ( (typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties )
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

SwapchainSupportDetails querySwapchainSupport ( VkPhysicalDevice device, VkSurfaceKHR surface )
{
	SwapchainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR ( device, surface, &details.capabilities );
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR ( device, surface, &formatCount, nullptr );
	if ( formatCount )
	{
		details.formats.resize ( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR ( device, surface, &formatCount, details.formats.data () );
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR ( device, surface, &presentModeCount, nullptr );
	if ( presentModeCount )
	{
		details.presentModes.resize ( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR ( device, surface, &presentModeCount, details.presentModes.data () );
	}
	return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat ( const std::vector<VkSurfaceFormatKHR>& availableFormats )
{
	return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
}

VkPresentModeKHR chooseSwapPresentMode ( const std::vector<VkPresentModeKHR>& availablePresentModes )
{
	for ( const auto mode : availablePresentModes )
	{
		if ( mode == VK_PRESENT_MODE_MAILBOX_KHR ) return mode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t chooseSwapImageCount ( const VkSurfaceCapabilitiesKHR& caps )
{
	const uint32_t imageCount = caps.minImageCount + 1;
	const bool imageCountExceeded = caps.maxImageCount && imageCount > caps.maxImageCount;
	return imageCountExceeded ? caps.maxImageCount : imageCount;
}

VkResult createSwapchain ( 
	VkDevice device, 
	VkPhysicalDevice physicalDevice, 
	VkSurfaceKHR surface, 
	uint32_t graphicsFamily, 
	uint32_t width, uint32_t height, 
	VkSwapchainKHR* swapchain )
{
	auto swapchainSupport = querySwapchainSupport ( physicalDevice, surface );
	auto surfaceFormat = chooseSwapSurfaceFormat ( swapchainSupport.formats );
	auto presentMode = chooseSwapPresentMode ( swapchainSupport.presentModes );

	const VkSwapchainCreateInfoKHR ci = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.flags = 0,
		.surface = surface,
		.minImageCount = chooseSwapImageCount ( swapchainSupport.capabilities ),
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = {
			.width = width,
			.height = height
		},
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &graphicsFamily,
		.preTransform = swapchainSupport.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};
	return vkCreateSwapchainKHR ( device, &ci, nullptr, swapchain );
}

size_t createSwapchainImages ( 
	VkDevice device, 
	VkSwapchainKHR swapchain, 
	std::vector<VkImage>& swapchainImages, 
	std::vector<VkImageView>& swapchainImageViews )
{
	uint32_t imageCount = 0;
	VK_ASSERT ( vkGetSwapchainImagesKHR ( device, swapchain, &imageCount, nullptr ) == VK_SUCCESS );
	swapchainImages.resize ( imageCount );
	swapchainImageViews.resize ( imageCount );
	VK_ASSERT ( vkGetSwapchainImagesKHR ( device, swapchain, &imageCount, swapchainImages.data () ) == VK_SUCCESS );
	for ( unsigned i = 0; i < imageCount; i++ )
	{
		if ( !createImageView ( device, swapchainImages[i], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &swapchainImageViews[i] ) )
			exit ( EXIT_FAILURE );
	}
	return imageCount;
}

bool createDescriptorPool ( 
	VkDevice device, 
	uint32_t imageCount,
	uint32_t uniformBufferCount,
	uint32_t storageBufferCount, 
	uint32_t samplerCount, 
	VkDescriptorPool* descPool )
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	
	if ( uniformBufferCount )
	{
		poolSizes.push_back ( VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = imageCount * uniformBufferCount } );
	}
	
	if ( storageBufferCount )
	{
		poolSizes.push_back ( VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = imageCount * storageBufferCount } );
	}
	
	if ( samplerCount )
	{
		poolSizes.push_back ( VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = imageCount * samplerCount } );
	}

	const VkDescriptorPoolCreateInfo pi = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = static_cast<uint32_t>(imageCount),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size ()),
		.pPoolSizes = poolSizes.empty () ? nullptr : poolSizes.data () };

	VK_CHECK ( vkCreateDescriptorPool ( device, &pi, nullptr, descPool ) );
	return true;
}

bool createImageView ( 
	VkDevice device, 
	VkImage image, 
	VkFormat format, 
	VkImageAspectFlags aspectFlags, 
	VkImageView* imageView, 
	VkImageViewType viewType, 
	uint32_t layerCount, 
	uint32_t mipLevels )
{
	const VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = {
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	VK_CHECK ( vkCreateImageView ( device, &viewInfo, nullptr, imageView ) );
	return true;
}

bool createBuffer ( 
	VkDevice device, 
	VkPhysicalDevice physicalDevice, 
	VkDeviceSize size, 
	VkBufferUsageFlags usage, 
	VkMemoryPropertyFlags properties, 
	VkBuffer& buffer, 
	VkDeviceMemory& bufferMemory )
{
	const VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

	VK_CHECK ( vkCreateBuffer ( device, &bufferInfo, nullptr, &buffer ) );
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements ( device, buffer, &memRequirements );
	const VkMemoryAllocateInfo ai = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType ( physicalDevice, memRequirements.memoryTypeBits, properties )
	};
	VK_CHECK ( vkAllocateMemory ( device, &ai, nullptr, &bufferMemory ) );
	vkBindBufferMemory (
		/* VkDevice device */			device,
		/* VkBuffer buffer */			buffer,
		/* VkDeviceMemory memory */		bufferMemory,
		/* VkDeviceSize memoryOffset */	0
	);
	return true;
}

bool createImage ( 
	VkDevice device, 
	VkPhysicalDevice physicalDevice, 
	uint32_t width, uint32_t height, 
	VkFormat format, 
	VkImageTiling tiling,
	VkImageUsageFlags usage, 
	VkMemoryPropertyFlags properties, 
	VkImage& image, VkDeviceMemory& imageMemory )
{
	const VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = VkExtent3D {.width = width, .height = height, .depth = 1	},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	VK_CHECK ( vkCreateImage ( device, &imageInfo, nullptr, &image ) );
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements ( device, image, &memRequirements );
	const VkMemoryAllocateInfo ai = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType ( physicalDevice, memRequirements.memoryTypeBits, properties )
	};
	VK_CHECK ( vkAllocateMemory ( device, &ai, nullptr, &imageMemory ) );
	vkBindImageMemory ( device, image, imageMemory, 0 );
	return true;
}

void createDepthResources ( 
	VulkanRenderDevice& vkDev,
	uint32_t width, uint32_t height, 
	VulkanTexture& depth )
{
	VkFormat depthFormat = findDepthFormat ( vkDev.physicalDevice );
	createImage ( vkDev.device, vkDev.physicalDevice, width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth.image, depth.imageMemory );
	createImageView ( vkDev.device, depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &depth.imageView );
	transitionImageLayout ( vkDev, depth.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

bool createTextureSampler ( VkDevice device, VkSampler* sampler )
{
	const VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};
	VK_CHECK ( vkCreateSampler ( device, &samplerInfo, nullptr, sampler ) );
	return true;
}

bool createTextureImage (
	VulkanRenderDevice& vkDev, 
	const char* filename, 
	VkImage& textureImage,
	VkDeviceMemory& textureImageMemory )
{
	int texWidth, texHeight, texChannels;

	stbi_uc* pixels = stbi_load ( filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );

	VkDeviceSize imageSize = texWidth * texHeight * 4;
	if ( !pixels )
	{
		printf ( "Failed to load [%s] texture\n", filename );
		return false;
	}

	// A staging buffer is necessary to upload texture data into the GPU via memory mapping. This buffer should be declared as VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	createBuffer ( vkDev.device, vkDev.physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory );

	void* data;
	vkMapMemory ( vkDev.device, stagingMemory, 0, imageSize, 0, &data );
	memcpy ( data, pixels, static_cast<size_t>(imageSize) );
	vkUnmapMemory ( vkDev.device, stagingMemory );

	// The actual image is located in the device memory and can't be accessed directly from the host
	createImage ( vkDev.device, vkDev.physicalDevice, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory );
	transitionImageLayout ( vkDev, textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
	copyBufferToImage ( vkDev, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) );
	transitionImageLayout ( vkDev, textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	vkDestroyBuffer ( vkDev.device, stagingBuffer, nullptr );
	vkFreeMemory ( vkDev.device, stagingMemory, nullptr );
	stbi_image_free ( pixels );
	return true;
}

bool createTexturedVertexBuffer ( 
	VulkanRenderDevice& vkDev, 
	const char* filename,
	VkBuffer* storageBuffer, 
	VkDeviceMemory* storageBufferMemory, 
	size_t* vertexBufferSize,
	size_t* indexBufferSize )
{
	// load mesh via Assimp into Vulkan shader storage buffer

	const aiScene* scene = aiImportFile ( filename, aiProcess_Triangulate );
	if ( !scene || !scene->HasMeshes () )
	{
		printf ( "Unable to load %s\n", filename );
		exit ( 255 );
	}

	const aiMesh* mesh = scene->mMeshes[0];
	struct VertexData {
		vec3 pos;
		vec2 tc;
	};

	std::vector<VertexData> vertices;
	for ( unsigned i = 0; i != mesh->mNumVertices; i++ )
	{
		const aiVector3D v = mesh->mVertices[i];
		const aiVector3D t = mesh->mTextureCoords[0][i];
		vertices.push_back ( { vec3 ( v.x, v.z, v.y ), vec2 ( t.x, t.y ) } );
	}

	std::vector<unsigned int> indices;
	for ( unsigned i = 0; i != mesh->mNumFaces; i++ )
	{
		for ( unsigned j = 0; j != 3; j++ )
		{
			indices.push_back ( mesh->mNumFaces[i].mIndices[j] );
		}
	}

	aiReleaseImport ( scene );

	// we need a staging buffer to upload the data into the GPU memory
	*vertexBufferSize = sizeof ( VertexData ) * vertices.size ();
	*indexBufferSize = sizeof ( unsigned int ) * indices.size ();
	VkDeviceSize bufferSize = *vertexBufferSize + *indexBufferSize;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	createBuffer ( vkDev.device, vkDev.physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory );
	
	void* data;
	vkMapMemory ( vkDev.device, stagingMemory, 0, bufferSize, 0, &data );
	// copy vertices to mapped data
	memcpy ( data, vertices.data (), *vertexBufferSize );
	// copy indices to data with an offset of the size of vertices
	memcpy ( (unsigned char*)data + *vertexBufferSize, indices.data (), *indexBufferSize );
	vkUnmapMemory ( vkDev.device, stagingMemory );
	createBuffer ( vkDev.device, vkDev.physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *storageBuffer, *storageBufferMemory );
	// copy the staging buffer to our storage buffer
	copyBuffer ( vkDev, stagingBuffer, *storageBuffer, bufferSize );
	vkDestroyBuffer ( vkDev.device, stagingbuffer, nullptr );
	vkFreeMemory ( vkDev.device, stagingMemory, nullptr );
	return true;
}

bool setVkObjectName ( 
	VulkanRenderDevice& vkDev, 
	void* object, 
	VkObjectType objType, 
	const char* name )
{
	VkDebugUtilsObjectNameInfoEXT nameInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext = nullptr,
		.objectType = objType,
		.objectHandle = (uint64_t)object,
		.pObjectName = name
	};
	return (vkSetDebugUtilsObjectNameEXT ( vkDev.device, &nameInfo ) == VK_SUCCESS);
}

VkResult createSemaphore ( 
	VkDevice device, 
	VkSemaphore* outSemaphore )
{
	const VkSemaphoreCreateInfo ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	return vkCreateSemaphore ( device, &ci, nullptr, outSemaphore );
}

bool initVulkanRenderDevice (
	VulkanInstance& vk, 
	VulkanRenderDevice& vkDev, 
	uint32_t width, 
	uint32_t height,
	std::function<bool ( VkPhysicalDevice )> selector, 
	VkPhysicalDeviceFeatures deviceFeatures )
{
	VK_CHECK ( findSuitablePhysicalDevice ( vk.instance, selector, &vkDev.physicalDevice ) );
	vkDev.graphicsFamily = findQueueFamilies ( vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT );
	VK_CHECK ( createDevice ( vkDev.physicalDevice, deviceFeatures, vkDev.graphicsFamily, &vkDev.device ) );
	vkGetDeviceQueue ( vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue );
	if ( vkDev.graphicsQueue = nullptr ) exit ( EXIT_FAILURE );
	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR ( vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported );
	if ( !presentSupported ) exit ( EXIT_FAILURE );
	VK_CHECK ( createSwapchain ( vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain ) );
	const size_t imageCount = createSwapchainImages ( vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews );
	vkDev.commandBuffers.resize ( imageCount );
	VK_CHECK ( createSemaphore ( vkDev.device, &vkDev.semaphore ) );
	VK_CHECK ( createSemaphore ( vkDev.device, &vkDev.renderSemaphore ) );
	const VkCommandPoolCreateInfo cpi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = 0,
		.queueFamilyIndex = vkDev.graphicsFamily
	};
	VK_CHECK ( vkCreateCommandPool ( vkDev.device, &cpi, nullptr, &vkDev.commandPool ) );
	const VkCommandBufferAllocateInfo ai = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vkDev.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (uint32_t)(vkDev.swapchainImages.size ())
	};
	VK_CHECK ( vkAllocateCommandBuffers ( vkDev.device, &ai, &vkDev.commandBuffers[0] ) );
	return true;
}

void destroyVulkanRenderDevice ( VulkanRenderDevice& vkDev )
{
	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		vkDestroyImageView ( vkDev.device, vkDev.swapchainImageViews[i], nullptr );
	}

	vkDestroySwapchainKHR ( vkDev.device, vkDev.swapchain, nullptr );
	vkDestroyCommandPool ( vkDev.device, vkDev.commandPool, nullptr );
	vkDestroySemaphore ( vkDev.device, vkDev.semaphore, nullptr );
	vkDestroySemaphore ( vkDev.device, vkDev.renderSemaphore, nullptr );

	vkDestroyDevice ( vkDev.device, nullptr );
}

void destroyVulkanInstance ( VulkanInstance& vk )
{
	vkDestroySurfaceKHR ( vk.instance, vk.surface, nullptr );
	vkDestroyDebugReportCallbackEXT ( vk.instance, vk.reportCallback, nullptr );
	vkDestroyDebugUtilsMessengerEXT ( vk.instance, vk.messenger, nullptr );
	vkDestroyInstance ( vk.instance, nullptr );
}

void destroyVulkanTexture ( VkDevice device, VulkanTexture& texture )
{
	vkDestroyImageView ( device, texture.imageView, nullptr );
	vkDestroyImage ( device, texture.image, nullptr );
	vkFreeMemory ( device, texture.imageMemory, nullptr );
}

uint32_t findMemoryType ( 
	VkPhysicalDevice device, 
	uint32_t typeFilter, 
	VkMemoryPropertyFlags properties )
{
	VkPhysicalDeviceProperties memProperties;

}
 
// begin/end SingleTimecommands using aggregate structures

VkCommandBuffer beginSingleTimeCommands ( VulkanRenderDevice& vkDev )
{
	VkCommandBuffer commandBuffer;

	const VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vkDev.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	vkAllocateCommandBuffers ( vkDev.device, &allocInfo, &commandBuffer );

	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer ( commandBuffer, &beginInfo );
	
	return commandBuffer;
}

void endSingleTimeCommands ( VulkanRenderDevice& vkDev, VkCommandBuffer commandBuffer )
{
	vkEndCommandBuffer ( commandBuffer );

	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};

	vkQueueSubmit ( vkDev.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle ( vkDev.graphicsQueue );

	vkFreeCommandBuffers ( vkDev.device, vkDev.commandPool, 1, &commandBuffer );
}

void copyBuffer ( 
	VulkanRenderDevice& vkDev, 
	VkBuffer srcBuffer, 
	VkBuffer dstBuffer, 
	VkDeviceSize size )
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands ( vkDev );

	const VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};

	vkCmdCopyBuffer ( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );

	endSingleTimeCommands ( vkDev, commandBuffer );
}

void copyBufferToImage ( 
	VulkanRenderDevice& vkDev, 
	VkBuffer buffer, 
	VkImage image, 
	uint32_t width, uint32_t height )
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands ( vkDev );

	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
		.imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
		.imageExtent = VkExtent3D {.width = width, .height = height, .depth = 1 }
	};

	vkCmdCopyBufferToImage ( commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
	endSingleTimeCommands ( vkDev, commandBuffer );
}

void transitionImageLayout ( 
	VulkanRenderDevice& vkDev, 
	VkImage image, 
	VkFormat format, 
	VkImageLayout oldLayout, 
	VkImageLayout newLayout, 
	uint32_t layerCount, 
	uint32_t mipLevels )
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands ( vkDev );
	transitionImageLayoutCmd ( commandBuffer, image, format, oldLayout, newLayout, layerCount, mipLevels );
	endSingleTimeCommands ( vkDev, commandBuffer );
}

void transitionImageLayoutCmd ( 
	VkCommandBuffer commandBuffer, 
	VkImage image, 
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout, 
	uint32_t layerCount, 
	uint32_t mipLevels )
{
	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = VkImageSubresourceRange {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
	};

	VkPipelineStageFlags sourceStage, destinationStage;
	if ( newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if ( hasStencilComponent ( format ) )
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		} 		
	} else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	vkCmdPipelineBarrier (
		/* VkCommandBuffer commandBuffer */							commandBuffer,
		/* VkPipelineStageFlags srcStageMask */						sourceStage,
		/* VkPipelineStageFlags dstStageMask */						destinationStage,
		/* VkDependencyFlags dependencyFlags */						0,
		/* uint32_t memoryBarrierCount */							0,
		/* const VkMemoryBarrier *pMemoryBarriers */				nullptr,
		/* uint32_t buffermemoryBarrierCount */						0,
		/* const vkBufferMemoryBarrier *pBufferMemoryBarriers */	nullptr,
		/* uint32_t imageMemoryBarrierCount */						1,
		/* const VkImageMemoryBarrier *pImageMemoryBarriers */		&barrier
		);


}

/*

VkCommandBuffer beginSingleTimeCommands ( VkDevice device, VkCommandPool commandPool, VkQueue queue )
{
	VkCommandBuffer commandBuffer;
	const VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	vkAllocateCommandBuffers ( device, &allocInfo, &commandBuffer );
	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};
	vkBeginCommandBuffer ( commandBuffer, &beginInfo );
	return commandBuffer;
}

void endSingleTimeCommands ( VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer )
{
	vkEndCommandBuffer ( commandBuffer );
	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};
	vkQueueSubmit ( graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle ( graphicsQueue );
	vkFreeCommandBuffers ( device, commandPool, 1, &commandBuffer );
}

void copyBuffer ( 
	VkDevice device,
	VkCommandPool commandPool,
	VkQueue graphicsQueue,
	VkBuffer srcBuffer,	VkBuffer dstBuffer, 
	VkDeviceSize size )
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands ( device, commandPool, graphicsQueue );
	const VkBufferCopy copyParam = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};
	vkCmdCopyBuffer ( commandBuffer, srcBuffer, dstBuffer, 1, &copyParam );
	endSingleTimeCommands ( device, commandPool, graphicsQueue, commandBuffer );
}

void copyBufferToImage ( VkDevice device, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height )
{
	
}

*/