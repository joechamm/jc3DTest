#include <jcVkFramework/Utils.h>
#include <jcVkFramework/UtilsVulkan.h>
#include <jcVkFramework/Bitmap.h>
#include <jcVkFramework/UtilsCubemap.h>
#include <jcVkFramework/EasyProfilerWrapper.h>

#include <StandAlone/ResourceLimits.h>

#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <iostream>

using std::cout;
using std::endl;
using std::exception;

void CHECK ( bool check, const char* filename, int linenumber )
{
	if ( !check )
	{
		printf ( "CHECK() failed at %s:%i\n", filename, linenumber );
		assert ( false );
		exit ( EXIT_FAILURE );
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback ( 
	VkDebugUtilsMessageSeverityFlagBitsEXT Severity, 
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, 
	void* UserData )
{
	printf ( "Validation layer: %s\n", CallbackData->pMessage );
	return VK_FALSE;
}

bool setupDebugCallbacks ( 
	VkInstance instance, 
	VkDebugUtilsMessengerEXT* messenger )
{
	const VkDebugUtilsMessengerCreateInfoEXT ci = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = &VulkanDebugCallback,
			.pUserData = nullptr
	};

	VK_CHECK ( vkCreateDebugUtilsMessengerEXT ( instance, &ci, nullptr, messenger ) );
	
	return true;
}

void populateDebugMessengerCreateInfo ( VkDebugUtilsMessengerCreateInfoEXT& createInfo )
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = VulkanDebugCallback;
}

VkShaderStageFlagBits glslangShaderStageToVulkan ( glslang_stage_t sh )
{
	switch ( sh )
	{
	case GLSLANG_STAGE_VERTEX:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case GLSLANG_STAGE_FRAGMENT:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case GLSLANG_STAGE_GEOMETRY:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case GLSLANG_STAGE_TESSCONTROL:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case GLSLANG_STAGE_TESSEVALUATION:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case GLSLANG_STAGE_COMPUTE:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	}

	return VK_SHADER_STAGE_VERTEX_BIT;
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

static_assert(sizeof ( TBuiltInResource ) == sizeof ( glslang_resource_t ));

static size_t compileShader ( glslang_stage_t stage, const char* shaderSource, ShaderModule& shaderModule )
{
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
		.resource = (const glslang_resource_t*)&glslang::DefaultTBuiltInResource,
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

	{
		const char* spirv_messages = glslang_program_SPIRV_get_messages ( prog );

		if ( spirv_messages )
		{
			fprintf ( stderr, "%s", spirv_messages );
		}
	}

	glslang_program_delete ( prog );
	glslang_shader_delete ( shd );

	return shaderModule.SPIRV.size ();
}

size_t compileShaderFile ( const char* file, ShaderModule& shaderModule )
{
	if ( auto shaderSource = readShaderFile ( file ); !shaderSource.empty () )
	{
		return compileShader ( glslangShaderStageFromFileName ( file ), shaderSource.c_str (), shaderModule );
	}

	return 0;		
}

VkResult createShaderModule ( VkDevice device, ShaderModule* shader, const char* filename )
{
	if ( compileShaderFile ( filename, *shader ) < 1 )
	{
		return VK_NOT_READY;
	}

	const VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shader->SPIRV.size() * sizeof(unsigned int),
		.pCode = shader->SPIRV.data()
	};

	return vkCreateShaderModule ( device, &createInfo, nullptr, &shader->shaderModule );
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
		/* for indexed textures */
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
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

	VkResult createInstanceResult = vkCreateInstance ( &createInfo, nullptr, instance );

	if ( createInstanceResult != VK_SUCCESS )
	{
		printf ( "Failed to create Vulkan Instance with result %s\n", vulkanResultToString ( createInstanceResult ).c_str () );
		exit ( EXIT_FAILURE );
	}

//	VK_CHECK ( vkCreateInstance ( &createInfo, nullptr, instance )  );

	volkLoadInstance ( *instance );
}

void createInstanceWithDebugging ( VkInstance* instance, const char* appName )
{
	std::vector<const char*> validationLayers;

#ifdef _DEBUG
	validationLayers.push_back ( "VK_LAYER_KHRONOS_validation" );
#endif

	std::vector<const char*> requiredExtensions = getGLFWRequiredExtensions ();

#ifdef _DEBUG
	requiredExtensions.push_back ( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
#endif

	requiredExtensions.push_back ( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );

	if ( !checkValidationLayerSupport ( validationLayers ) )
	{
		cout << "Validation Layer Support Not Available" << endl;
		exit ( EXIT_FAILURE );
	}

	if ( !checkExtensionSupport ( requiredExtensions ) )
	{
		cout << "Extension Support Not Available" << endl;
		exit ( EXIT_FAILURE );
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName;
	appInfo.applicationVersion = VK_MAKE_VERSION ( 1, 0, 0 );
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION ( 1, 0, 0 );
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size ());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data ();

	if ( validationLayers.empty () )
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}
	else
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size ());
		createInfo.ppEnabledLayerNames = validationLayers.data ();
	}

#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
	populateDebugMessengerCreateInfo ( debugCreateInfo );
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#endif

	VkResult createInstanceResult = vkCreateInstance ( &createInfo, nullptr, instance );

	if ( createInstanceResult != VK_SUCCESS )
	{
		printf ( "Failed to create Vulkan Instance with result %s\n", vulkanResultToString ( createInstanceResult ).c_str () );
		exit ( EXIT_FAILURE );
	}

//	VK_CHECK ( vkCreateInstance ( &createInfo, nullptr, instance )  );

	volkLoadInstance ( *instance );
}

VkResult createDevice (
	VkPhysicalDevice physicalDevice,
	VkPhysicalDeviceFeatures deviceFeatures,
	uint32_t graphicsFamily,
	VkDevice* device )
{
	const std::vector<const char*> extensions = 
	{ 
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
	};

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

VkResult createDevice2 ( VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatures2, uint32_t graphicsFamily, VkDevice* device )
{
	const std::vector<const char*> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		// for legacy drivers Vulkan 1.1
		VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME
	};

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
		.pNext = &deviceFeatures2,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &qci,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size ()),
		.ppEnabledExtensionNames = extensions.data (),
		.pEnabledFeatures = nullptr
	};

	return vkCreateDevice ( physicalDevice, &ci, nullptr, device );
}

VkResult createDeviceWithCompute ( VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily, uint32_t computeFamily, VkDevice* device )
{
	const std::vector<const char*> extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	/* If we use a single queue, we can call the old device initialization routine */
	if ( graphicsFamily == computeFamily )
	{
		return createDevice ( physicalDevice, deviceFeatures, graphicsFamily, device );
	}

	/* If there are two distinct queues, fill in two individual "VkDeviceQueueCreateInfo" structures */
	const float queuePriorities[2] = { 0.0f, 0.0f };

	/* The graphics queue creation structure refers to the graphics queue family index */
	const VkDeviceQueueCreateInfo qciGfx = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriorities[0]
	};

	/* The compute queue creation is similiar  */
	const VkDeviceQueueCreateInfo qciComp = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = computeFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriorities[1]
	};

	/* Both queue creation structures should be stored in an array */
	const VkDeviceQueueCreateInfo qci[] = { qciGfx, qciComp };

	/* The device creation structure now uses two references to the graphics and compute queues */
	const VkDeviceCreateInfo ci = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 2,
		.pQueueCreateInfos = qci,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size ()),
		.ppEnabledExtensionNames = extensions.data (),
		.pEnabledFeatures = &deviceFeatures
	};

	return vkCreateDevice ( physicalDevice, &ci, nullptr, device );
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
	VK_CHECK ( vkGetSwapchainImagesKHR ( device, swapchain, &imageCount, nullptr ) );

	swapchainImages.resize ( imageCount );
	swapchainImageViews.resize ( imageCount );
	
	VK_CHECK ( vkGetSwapchainImagesKHR ( device, swapchain, &imageCount, swapchainImages.data () ) );

	for ( unsigned i = 0; i < imageCount; i++ )
	{
		if ( !createImageView ( device, swapchainImages[i], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &swapchainImageViews[i] ) )
		{
			exit ( EXIT_FAILURE );
		}
		
	}
	return static_cast<size_t>(imageCount);
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
	vkDev.framebufferWidth = width;
	vkDev.framebufferHeight = height;

	VK_CHECK ( findSuitablePhysicalDevice ( vk.instance, selector, &vkDev.physicalDevice ) );
	vkDev.graphicsFamily = findQueueFamilies ( vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT );
	VK_CHECK ( createDevice ( vkDev.physicalDevice, deviceFeatures, vkDev.graphicsFamily, &vkDev.device ) );

	vkGetDeviceQueue ( vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue );
	if ( vkDev.graphicsQueue == nullptr )
	{
		exit ( EXIT_FAILURE );
	}
		
	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR ( vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported );
	if ( !presentSupported )
	{
		exit ( EXIT_FAILURE );
	}

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
		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size ())
	};

	VK_CHECK ( vkAllocateCommandBuffers ( vkDev.device, &ai, &vkDev.commandBuffers[0] ) );
	
	return true;
}

bool initVulkanRenderDevice2 ( VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool ( VkPhysicalDevice )> selector, VkPhysicalDeviceFeatures2 deviceFeatures2 )
{
	vkDev.framebufferWidth = width;
	vkDev.framebufferHeight = height;

	VK_CHECK ( findSuitablePhysicalDevice ( vk.instance, selector, &vkDev.physicalDevice ) );
	vkDev.graphicsFamily = findQueueFamilies ( vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT );
	VK_CHECK ( createDevice2 ( vkDev.physicalDevice, deviceFeatures2, vkDev.graphicsFamily, &vkDev.device ) );

	vkGetDeviceQueue ( vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue );
	if ( vkDev.graphicsQueue == nullptr )
	{
		exit ( EXIT_FAILURE );
	}

	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR ( vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported );
	if ( !presentSupported )
	{
		exit ( EXIT_FAILURE );
	}

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
		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size ())
	};

	VK_CHECK ( vkAllocateCommandBuffers ( vkDev.device, &ai, &vkDev.commandBuffers[0] ) );

	return true;
}

bool initVulkanRenderDeviceWithCompute ( VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkPhysicalDeviceFeatures deviceFeatures )
{
	vkDev.framebufferWidth = width;
	vkDev.framebufferHeight = height;

	/*
	*	After finding the physical device and graphics queue, we also search for the compute-capable queue. 
	*	This code will find a combined graphics plus compute queue even on devices that support a separate compute queue, as the combined queue tends to have a lower index. 
	*	For simplicity, we use this approach throughout the book
	*/
	VK_CHECK ( findSuitablePhysicalDevice ( vk.instance, &isDeviceSuitable, &vkDev.physicalDevice ) );
	vkDev.graphicsFamily = findQueueFamilies ( vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT );
	vkDev.computeFamily = findQueueFamilies ( vkDev.physicalDevice, VK_QUEUE_COMPUTE_BIT );

	/* To initialize both queues, or a single one if these queues are the same, we call a new function, "createDeviceWithCompute()" */
	VK_CHECK ( createDeviceWithCompute ( vkDev.physicalDevice, deviceFeatures, vkDev.graphicsFamily, vkDev.computeFamily, &vkDev.device ) );

	/* Save unique queue indices for later use in the "createSharedBuffer()" routine */
	vkDev.deviceQueueIndices.push_back ( vkDev.graphicsFamily );
	if ( vkDev.graphicsFamily != vkDev.computeFamily )
	{
		vkDev.deviceQueueIndices.push_back ( vkDev.computeFamily );
	}

	/* After saving queue indices, we acquire the graphics and compute queue handles */
	vkGetDeviceQueue ( vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue );
	if ( !vkDev.graphicsQueue ) exit ( EXIT_FAILURE );
	vkGetDeviceQueue ( vkDev.device, vkDev.computeFamily, 0, &vkDev.computeQueue );
	if ( !vkDev.computeQueue ) exit ( EXIT_FAILURE );

	/* After initializing the queues, we create everything related to the swapchain.*/
	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR ( vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported );
	if ( !presentSupported ) exit ( EXIT_FAILURE );

	VK_CHECK ( createSwapchain ( vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain ) );
	const size_t imageCount = createSwapchainImages ( vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews );
	vkDev.commandBuffers.resize ( imageCount );

	/* The rendering synchronization primitives and a command buffer with a command pool are also created the same way as in the Chapter 5 Indirect Rendering in Vulkan exercise */
	VK_CHECK ( createSemaphore ( vkDev.device, &vkDev.semaphore ) );
	VK_CHECK ( createSemaphore ( vkDev.device, &vkDev.renderSemaphore ) );

	/* For each swapchain image, we create a separate command queue */
	const VkCommandPoolCreateInfo cpi1 = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = 0,
		.queueFamilyIndex = vkDev.graphicsFamily
	};

	VK_CHECK ( vkCreateCommandPool ( vkDev.device, &cpi1, nullptr, &vkDev.commandPool ) );

	const VkCommandBufferAllocateInfo ai1 = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vkDev.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size ())
	};

	VK_CHECK ( vkAllocateCommandBuffers ( vkDev.device, &ai1, &vkDev.commandBuffers[0] ) );

	/* create a single command pool for the compute queue */
	const VkCommandPoolCreateInfo cpi2 = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = vkDev.computeFamily
	};

	VK_CHECK ( vkCreateCommandPool ( vkDev.device, &cpi2, nullptr, &vkDev.computeCommandPool ) );

	/* Using the created command pool, we allocate the command buffer for compute shaders */
	const VkCommandBufferAllocateInfo ai2 = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vkDev.computeCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	VK_CHECK ( vkAllocateCommandBuffers ( vkDev.device, &ai2, &vkDev.computeCommandBuffer ) );

	vkDev.useCompute = true;
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

	if ( vkDev.useCompute )
	{
		vkDestroyCommandPool ( vkDev.device, vkDev.computeCommandPool, nullptr );
	}

	vkDestroyDevice ( vkDev.device, nullptr );
}

void destroyVulkanInstance ( VulkanInstance& vk )
{
	vkDestroySurfaceKHR ( vk.instance, vk.surface, nullptr );
	vkDestroyDebugUtilsMessengerEXT ( vk.instance, vk.messenger, nullptr );
	vkDestroyInstance ( vk.instance, nullptr );
}

bool createDescriptorPool ( VulkanRenderDevice& vkDev, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descriptorPool )
{
	const uint32_t imageCount = static_cast<uint32_t>(vkDev.swapchainImages.size ());

	std::vector<VkDescriptorPoolSize> poolSizes;

	if ( uniformBufferCount )
	{
		poolSizes.push_back ( VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = imageCount * uniformBufferCount
		} );
	}

	if ( storageBufferCount )
	{
		poolSizes.push_back ( VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = imageCount * storageBufferCount
		} );
	}

	if ( samplerCount )
	{
		poolSizes.push_back ( VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = imageCount * samplerCount
		} );
	}

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = static_cast<uint32_t>(imageCount),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size ()),
		.pPoolSizes = poolSizes.empty () ? nullptr : poolSizes.data ()
	};

	return (vkCreateDescriptorPool ( vkDev.device, &poolInfo, nullptr, descriptorPool ) == VK_SUCCESS);
}

//
//bool createDescriptorPool ( 
//	VkDevice device, 
//	uint32_t imageCount,
//	uint32_t uniformBufferCount,
//	uint32_t storageBufferCount, 
//	uint32_t samplerCount, 
//	VkDescriptorPool* descPool )
//{
//	std::vector<VkDescriptorPoolSize> poolSizes;
//	
//	if ( uniformBufferCount )
//	{
//		poolSizes.push_back ( VkDescriptorPoolSize{
//			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//			.descriptorCount = imageCount * uniformBufferCount } );
//	}
//	
//	if ( storageBufferCount )
//	{
//		poolSizes.push_back ( VkDescriptorPoolSize{
//			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
//			.descriptorCount = imageCount * storageBufferCount } );
//	}
//	
//	if ( samplerCount )
//	{
//		poolSizes.push_back ( VkDescriptorPoolSize{
//			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//			.descriptorCount = imageCount * samplerCount } );
//	}
//
//	const VkDescriptorPoolCreateInfo pi = {
//		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.maxSets = static_cast<uint32_t>(imageCount),
//		.poolSizeCount = static_cast<uint32_t>(poolSizes.size ()),
//		.pPoolSizes = poolSizes.empty () ? nullptr : poolSizes.data () };
//
//	VK_CHECK ( vkCreateDescriptorPool ( device, &pi, nullptr, descPool ) );
//	return true;
//}

bool isDeviceSuitable ( VkPhysicalDevice device )
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties ( device, &deviceProperties );

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures ( device, &deviceFeatures );

	const bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	const bool isIntegratedGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
	const bool isGPU = isDiscreteGPU || isIntegratedGPU;

	return isGPU && deviceFeatures.geometryShader;
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
	const bool imageCountExceeded = caps.maxImageCount > 0 && imageCount > caps.maxImageCount;
	return imageCountExceeded ? caps.maxImageCount : imageCount;
}

VkResult findSuitablePhysicalDevice (
	VkInstance instance,
	std::function<bool ( VkPhysicalDevice )> selector,
	VkPhysicalDevice* physicalDevice )
{
	uint32_t deviceCount = 0;
	VK_CHECK_RET ( vkEnumeratePhysicalDevices ( instance, &deviceCount, nullptr ) );

	if ( !deviceCount )
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}

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
	for ( VkFormat format : candidates )
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties ( device, format, &props );

		if ( tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features )
		{
			return format;
		} else if ( tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features )
		{
			return format;
		}
	}

	printf ( "failed to find supported format!\n" );
	exit ( 0 );
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


VkFormat findDepthFormat ( VkPhysicalDevice device )
{
	return findSupportedFormat ( device, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

bool hasStencilComponent ( VkFormat format )
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool createGraphicsPipeline (
	VulkanRenderDevice& vkDev,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<const char*>& shaderFiles,
	VkPipeline* pipeline,
	VkPrimitiveTopology topology,
	bool useDepth,
	bool useBlending,
	bool dynamicScissorState,
	int32_t customWidth,
	int32_t customHeight,
	uint32_t numPatchControlPoints
)
{
	std::vector<ShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderStages.resize ( shaderFiles.size () );
	shaderModules.resize ( shaderFiles.size () );

	shaderStages.resize ( shaderFiles.size () );
	shaderModules.resize ( shaderFiles.size () );

	for ( size_t i = 0; i < shaderFiles.size (); i++ )
	{
		const char* file = shaderFiles[i];
		VK_CHECK ( createShaderModule ( vkDev.device, &shaderModules[i], file ) );

		VkShaderStageFlagBits stage = glslangShaderStageToVulkan ( glslangShaderStageFromFileName ( file ) );

		shaderStages[i] = shaderStageInfo ( stage, shaderModules[i], "main" );
	}

	const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		/* The only difference from createGraphicsPipeline() */
		.topology = topology,
		.primitiveRestartEnable = VK_FALSE
	};

	const VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(customWidth > 0 ? customWidth : vkDev.framebufferWidth),
		.height = static_cast<float>(customHeight > 0 ? customHeight : vkDev.framebufferHeight),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	const VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = { customWidth > 0 ? customWidth : vkDev.framebufferWidth, customHeight > 0 ? customHeight : vkDev.framebufferHeight }
	};

	const VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	const VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f
	};

	const VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f
	};

	const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	const VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	const VkPipelineDepthStencilStateCreateInfo depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
		.depthWriteEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};

	VkDynamicState dynamicStateElt = VK_DYNAMIC_STATE_SCISSOR;

	const VkPipelineDynamicStateCreateInfo dynamicState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.dynamicStateCount = 1,
		.pDynamicStates = &dynamicStateElt
	};

	const VkPipelineTessellationStateCreateInfo tessellationState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.patchControlPoints = numPatchControlPoints
	};

	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStages.size ()),
		.pStages = shaderStages.data (),
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = (topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &tessellationState : nullptr,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = useDepth ? &depthStencil : nullptr,
		.pColorBlendState = &colorBlending,
		.pDynamicState = dynamicScissorState ? &dynamicState : nullptr,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	VK_CHECK ( vkCreateGraphicsPipelines ( vkDev.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline ) );

	for ( auto m : shaderModules )
		vkDestroyShaderModule ( vkDev.device, m.shaderModule, nullptr );

	return true;
}

bool createGraphicsPipeline (
	VulkanRenderDevice& vkDev,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<std::string>& shaderFiles,
	VkPipeline* pipeline,
	VkPrimitiveTopology topology,
	bool useDepth,
	bool useBlending,
	bool dynamicScissorState,
	int32_t customWidth,
	int32_t customHeight,
	uint32_t numPatchControlPoints )
{
	std::vector<ShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderStages.resize ( shaderFiles.size () );
	shaderModules.resize ( shaderFiles.size () );

	shaderStages.resize ( shaderFiles.size () );
	shaderModules.resize ( shaderFiles.size () );

	for ( size_t i = 0; i < shaderFiles.size (); i++ )
	{
		const char* file = shaderFiles[i].c_str();
		VK_CHECK ( createShaderModule ( vkDev.device, &shaderModules[i], file ) );

		VkShaderStageFlagBits stage = glslangShaderStageToVulkan ( glslangShaderStageFromFileName ( file ) );

		shaderStages[i] = shaderStageInfo ( stage, shaderModules[i], "main" );
	}

	const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		/* The only difference from createGraphicsPipeline() */
		.topology = topology,
		.primitiveRestartEnable = VK_FALSE
	};

	const VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(customWidth > 0 ? customWidth : vkDev.framebufferWidth),
		.height = static_cast<float>(customHeight > 0 ? customHeight : vkDev.framebufferHeight),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	const VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = { customWidth > 0 ? customWidth : vkDev.framebufferWidth, customHeight > 0 ? customHeight : vkDev.framebufferHeight }
	};

	const VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	const VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f
	};

	const VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f
	};

	const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	const VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	const VkPipelineDepthStencilStateCreateInfo depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
		.depthWriteEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};

	VkDynamicState dynamicStateElt = VK_DYNAMIC_STATE_SCISSOR;

	const VkPipelineDynamicStateCreateInfo dynamicState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.dynamicStateCount = 1,
		.pDynamicStates = &dynamicStateElt
	};

	const VkPipelineTessellationStateCreateInfo tessellationState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.patchControlPoints = numPatchControlPoints
	};

	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStages.size ()),
		.pStages = shaderStages.data (),
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = (topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &tessellationState : nullptr,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = useDepth ? &depthStencil : nullptr,
		.pColorBlendState = &colorBlending,
		.pDynamicState = dynamicScissorState ? &dynamicState : nullptr,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	VK_CHECK ( vkCreateGraphicsPipelines ( vkDev.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline ) );

	for ( auto m : shaderModules )
		vkDestroyShaderModule ( vkDev.device, m.shaderModule, nullptr );

	return true;
}

VkResult createComputePipeline ( VkDevice device, VkShaderModule computeShader, VkPipelineLayout pipelineLayout, VkPipeline* pipeline )
{
	/* 
	*	The compute pipeline contains a VK_SHADER_STAGE_COMPUTE_BIT single shader stage and an attached Vulkan shader module. 
	*	For the purpose of simplicity, all our compute shaders must have "main()" as their entry point.
	*/
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = computeShader,
			.pName = "main",
			.pSpecializationInfo = nullptr
		},
		.layout = pipelineLayout,
		.basePipelineHandle = 0,
		.basePipelineIndex = 0
	};

	return vkCreateComputePipelines ( device, 0, 1, &computePipelineCreateInfo, nullptr, pipeline );
}

/* Default DS layout for In/Out buffer pair */
bool createComputeDescriptorSetLayout ( VkDevice device, VkDescriptorSetLayout* descriptorSetLayout )
{
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] = {
		{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 }
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		0, 0, 2, descriptorSetLayoutBindings
	};

	return (vkCreateDescriptorSetLayout ( device, &descriptorSetLayoutCreateInfo, 0, descriptorSetLayout ) == VK_SUCCESS);
}

//
//bool createGraphicsPipeline (
//	VkDevice device,
//	uint32_t width, uint32_t height,
//	VkRenderPass renderPass,
//	VkPipelineLayout pipelineLayout,
//	const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
//	VkPipeline* pipeline )
//{
//
//	const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
//
//	// Since we are using programmable vertex pulling, the only thing we need to specify for the input assembly is the primitive topology type. We must disable the primitive restart capabilities that we won't be using.
//	const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
//		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
//		.primitiveRestartEnable = VK_FALSE
//	};
//
//	const VkViewport viewport = {
//		.x = 0.0f,
//		.y = 0.0f,
//		.width = static_cast<float>(width),
//		.height = static_cast<float>(height),
//		.minDepth = 0.0f,
//		.maxDepth = 1.0f
//	};
//
//	// scissor covers the entire viewport
//	const VkRect2D scissor = {
//		.offset = { 0, 0},
//		.extent = { width, height }
//	};
//
//	// combine viewport and scissor declarations to get the required viewport state
//	const VkPipelineViewportStateCreateInfo viewportState = {
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
//		.viewportCount = 1,
//		.pViewports = &viewport,
//		.scissorCount = 1,
//		.pScissors = &scissor
//	};
//	// no backface culling, front-face=cw
//	const VkPipelineRasterizationStateCreateInfo rasterizer = {
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
//		.polygonMode = VK_POLYGON_MODE_FILL,
//		.cullMode = VK_CULL_MODE_NONE,
//		.frontFace = VK_FRONT_FACE_CLOCKWISE,
//		.lineWidth = 1.0f
//	};
//	// disable multisampling
//	const VkPipelineMultisampleStateCreateInfo multisampling = {
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
//		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
//		.sampleShadingEnable = VK_FALSE,
//		.minSampleShading = 1.0f
//	};
//
//	// all the blending operations should be disabled. a color mask is required if we want to see any pixels that have been rendered
//	const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
//		.blendEnable = VK_FALSE,
//		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
//	};
//
//	// disable any logic operations
//	const VkPipelineColorBlendStateCreateInfo colorBlending = {
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
//		.logicOpEnable = VK_FALSE,
//		.logicOp = VK_LOGIC_OP_COPY,
//		.attachmentCount = 1,
//		.pAttachments = &colorBlendAttachment,
//		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
//	};
//
//	// enable depth test 
//	const VkPipelineDepthStencilStateCreateInfo depthStencil = {
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
//		.depthTestEnable = VK_TRUE,
//		.depthWriteEnable = VK_TRUE,
//		.depthCompareOp = VK_COMPARE_OP_LESS,
//		.depthBoundsTestEnable = VK_FALSE,
//		.minDepthBounds = 0.0f,
//		.maxDepthBounds = 1.0f
//	};
//
//	const VkGraphicsPipelineCreateInfo pipelineInfo = {
//		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
//		.stageCount = static_cast<uint32_t>(shaderStages.size ()),
//		.pStages = shaderStages.data (),
//		.pVertexInputState = &vertexInputInfo,
//		.pInputAssemblyState = &inputAssembly,
//		.pTessellationState = nullptr,
//		.pViewportState = &viewportState,
//		.pRasterizationState = &rasterizer,
//		.pMultisampleState = &multisampling,
//		.pDepthStencilState = &depthStencil,
//		.pColorBlendState = &colorBlending,
//		.layout = pipelineLayout,
//		.renderPass = renderPass,
//		.subpass = 0,
//		.basePipelineHandle = VK_NULL_HANDLE,
//		.basePipelineIndex = -1
//	};
//	VK_CHECK ( vkCreateGraphicsPipelines ( device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline ) );
//	return true;
//}

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

bool createSharedBuffer ( VulkanRenderDevice& vkDev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory )
{
	/* If we have a single queue for graphics and compute, we delegate all the work to our old createBuffer() routine */
	const size_t familyCount = vkDev.deviceQueueIndices.size ();
	if ( familyCount < 2u )
	{
		return createBuffer ( vkDev.device, vkDev.physicalDevice, size, usage, properties, buffer, bufferMemory );
	}

	/* Inside the buffer creation structure, we should designate this buffer as being accessible from multiple command queues and pass a list of all the respective queue indices */
	const VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr, .flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = (familyCount > 1u) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = static_cast<uint32_t>(familyCount),
		.pQueueFamilyIndices = (familyCount > 1u) ? vkDev.deviceQueueIndices.data () : nullptr
	};

	/* The buffer itself is created, but no memory is associated with it yet */
	VK_CHECK ( vkCreateBuffer ( vkDev.device, &bufferInfo, nullptr, &buffer ) );

	/* 
	*	The rest of the code allocates memory with specified parameters, just as in the createBuffer() routine. 
	*	To do this, we ask the Vulkan implementation which memory-block properties we should use for this buffer.	* 
	*/
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements ( vkDev.device, buffer, &memRequirements );

	/* In the allocation structure, we specify the physical buffer size and the exact memory heap type */
	const VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType ( vkDev.physicalDevice, memRequirements.memoryTypeBits, properties )
	};

	/* Memory allocation and buffer binding conclude this routine */
	VK_CHECK ( vkAllocateMemory ( vkDev.device, &allocInfo, nullptr, &bufferMemory ) );
	vkBindBufferMemory ( vkDev.device, buffer, bufferMemory, 0 );
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
	VkImage& image, VkDeviceMemory& imageMemory,
	VkImageCreateFlags flags, uint32_t mipLevels )
{
	const VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = VkExtent3D {.width = width, .height = height, .depth = 1	},
		.mipLevels = mipLevels,
		.arrayLayers = (uint32_t)((flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? 6 : 1),
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

bool createUniformBuffer ( VulkanRenderDevice& vkDev, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize )
{
	return createBuffer ( vkDev.device, vkDev.physicalDevice, bufferSize, 
	/* Usage flags */			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	/* Memory property flags */	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		buffer, bufferMemory );
}

void uploadBufferData ( VulkanRenderDevice& vkDev, const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, const void* data, const size_t dataSize )
{
	EASY_FUNCTION ();

	void* mappedData = nullptr;
	vkMapMemory ( vkDev.device, bufferMemory, deviceOffset, dataSize, 0, &mappedData );
	memcpy ( mappedData, data, dataSize );
	vkUnmapMemory ( vkDev.device, bufferMemory );
}

void downloadBufferData ( VulkanRenderDevice& vkDev, const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, void* outData, const size_t dataSize )
{
	void* mappedData = nullptr;
	vkMapMemory ( vkDev.device, bufferMemory, deviceOffset, dataSize, 0, &mappedData );
	memcpy ( outData, mappedData, dataSize );
	vkUnmapMemory ( vkDev.device, bufferMemory );
}

bool downloadImageData ( VulkanRenderDevice& vkDev, VkImage& textureImage, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, void* imageData, VkImageLayout sourceImageLayout )
{
	uint32_t bytesPerPixel = bytesPerTexFormat ( texFormat );

	VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
	VkDeviceSize imageSize = layerSize * layerCount;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer ( vkDev.device, vkDev.physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );

	transitionImageLayout ( vkDev, textureImage, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, layerCount );
	copyImageToBuffer ( vkDev, textureImage, stagingBuffer, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), layerCount );
	transitionImageLayout ( vkDev, textureImage, texFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sourceImageLayout, layerCount );

	downloadBufferData ( vkDev, stagingBufferMemory, 0, imageData, imageSize );

	vkDestroyBuffer ( vkDev.device, stagingBuffer, nullptr );
	vkFreeMemory ( vkDev.device, stagingBufferMemory, nullptr );

	return true;
}

bool createDepthResources (
	VulkanRenderDevice& vkDev,
	uint32_t width, uint32_t height,
	VulkanImage& depth )
{
	VkFormat depthFormat = findDepthFormat ( vkDev.physicalDevice );

	if ( !createImage ( vkDev.device, vkDev.physicalDevice, width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth.image, depth.imageMemory ) )
	{
		return false;
	}

	if ( !createImageView ( vkDev.device, depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &depth.imageView ) )
	{
		return false;
	}

	transitionImageLayout ( vkDev, depth.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );

	return true;
}

bool createPipelineLayout (
	VkDevice device,
	VkDescriptorSetLayout dsLayout,
	VkPipelineLayout* pipelineLayout )
{
	const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &dsLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	return (vkCreatePipelineLayout ( device, &pipelineLayoutInfo, nullptr, pipelineLayout ) == VK_SUCCESS);
}

bool createPipelineLayoutWithConstants ( VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout, uint32_t vtxConstSize, uint32_t fragConstSize )
{
	const VkPushConstantRange ranges[] = {
		{
			VK_SHADER_STAGE_VERTEX_BIT, // stageFlags
			0,							// offset
			vtxConstSize				// size
		},
		{
			VK_SHADER_STAGE_FRAGMENT_BIT,	// stageFlags
			vtxConstSize,					// offset
			fragConstSize
		}
	};

	uint32_t constSize = (vtxConstSize > 0) + (fragConstSize > 0);

	const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &dsLayout,
		.pushConstantRangeCount = constSize,
		.pPushConstantRanges = (constSize == 0) ? nullptr : (vtxConstSize > 0 ? ranges : &ranges[1])
	};

	return (vkCreatePipelineLayout ( device, &pipelineLayoutInfo, nullptr, pipelineLayout ) == VK_SUCCESS);
}

bool createTextureImageFromData ( VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory, void* imageData, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, VkImageCreateFlags flags )
{
	createImage ( vkDev.device, vkDev.physicalDevice, texWidth, texHeight, texFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, flags );

	return updateTextureImage ( vkDev, textureImage, textureImageMemory, texWidth, texHeight, texFormat, layerCount, imageData );
}

static void float24to32 ( int w, int h, const float* img24, float* img32 )
{
	const int numPixels = w * h;
	for ( int i = 0; i != numPixels; i++ )
	{
		*img32++ = *img24++;
		*img32++ = *img24++;
		*img32++ = *img24++;
		*img32++ = 1.0f;
	}
}

bool createCubeTextureImage ( VulkanRenderDevice& vkDev, const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width, uint32_t* height )
{
	int w, h, comp;
	const float* img = stbi_loadf ( filename, &w, &h, &comp, 3 );
	std::vector<float> img32 ( w * h * 4 );

	float24to32 ( w, h, img, img32.data () );

	if ( !img )
	{
		printf ( "Failed to load [%s] texture\n", filename );
		fflush ( stdout );
		return false;
	}

	stbi_image_free ( (void*)img );

	Bitmap in ( w, h, 4, eBitmapFormat_Float, img32.data () );
	Bitmap out = convertEquirectangularMapToVerticalCross ( in );

	Bitmap cube = convertVerticalCrossToCubeMapFaces ( out );

	if ( width && height )
	{
		*width = w;
		*height = h;
	}

	return createTextureImageFromData ( vkDev, textureImage, textureImageMemory, cube.data_.data (), cube.w_, cube.h_, VK_FORMAT_R32G32B32A32_SFLOAT, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT );

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



bool createColorAndDepthRenderPass ( VulkanRenderDevice& vkDev, bool useDepth, VkRenderPass* renderPass, const RenderPassCreateInfo& ci, VkFormat colorFormat )
{
	const bool offscreenInt = ci.flags_ & eRenderPassBit_Offscreen;
	const bool first = ci.flags_ & eRenderPassBit_First;
	const bool last = ci.flags_ & eRenderPassBit_Last;
	VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = colorFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		// if the loadOp field is changed to VK_ATTACHMENT_LOAD_OP_DONT_CARE, the framebuffer will not be cleared at the beginning of the render pass. this can be desirable if the frame has been composed with the output of another rendering pass
		.loadOp = offscreenInt ? VK_ATTACHMENT_LOAD_OP_LOAD : (ci.clearColor_ ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = first ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
		.finalLayout = last ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	
	const VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	// the depth attachment is handled in a similar way
	VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = useDepth ? findDepthFormat ( vkDev.physicalDevice ) : VK_FORMAT_D32_SFLOAT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = offscreenInt ? VK_ATTACHMENT_LOAD_OP_LOAD : (ci.clearDepth_ ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = ci.clearDepth_ ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	// The subpasses in a render pass automatically take care of image layout transitions. 
	// This render pass also specifies one subpass dependency, which instructs vulkan to prevent the transition from happening util it is actually necessary and allowed. 
	// This dependency only makes sense for color buffer writes. In case of an offscreen render pass, we use subpass dependencies for layout transitions. 
	if ( ci.flags_ & eRenderPassBit_Offscreen )
	{
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

//	const VkSubpassDependency dependency = {
	std::vector<VkSubpassDependency> dependencies = {
		{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
		}
	};

	// add two explicit dependencies which ensure all rendering operations are completed before this render pass and before the color attachment can be used in subsequent passes
	if ( ci.flags_ & eRenderPassBit_Offscreen )
	{
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		dependencies.resize ( 2 );
		dependencies[0] = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};
		dependencies[1] = {
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};
	}

	// The rendering pass consists of a single subpass that only uses color and depth buffers
	const VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = useDepth ? &depthAttachmentRef : nullptr,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	// Now use the two attachments we defined earlier
	std::array<VkAttachmentDescription, 2> attachments = {
		colorAttachment,
		depthAttachment
	};

	const VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(useDepth ? 2 : 1),
		.pAttachments = attachments.data (),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = static_cast<uint32_t>(dependencies.size ()),
		.pDependencies = dependencies.data ()
	};

	return (vkCreateRenderPass ( vkDev.device, &renderPassInfo, nullptr, renderPass ) == VK_SUCCESS);
}

bool createColorAndDepthFramebuffer (
	VulkanRenderDevice& vkDev, 
	uint32_t width, uint32_t height, 
	VkRenderPass renderPass, 
	VkImageView colorImageView, 
	VkImageView depthImageView, 
	VkFramebuffer* framebuffer )
{
	std::array<VkImageView, 2> attachments = { colorImageView, depthImageView };

	const VkFramebufferCreateInfo framebufferInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = renderPass,
		.attachmentCount = (depthImageView == VK_NULL_HANDLE) ? 1u : 2u,
		.pAttachments = attachments.data (),
		.width = vkDev.framebufferWidth,
		.height = vkDev.framebufferHeight,
		.layers = 1
	};

	return (vkCreateFramebuffer ( vkDev.device, &framebufferInfo, nullptr, framebuffer ) == VK_SUCCESS);
}

bool createColorAndDepthFramebuffers ( 
	VulkanRenderDevice& vkDev, 
	VkRenderPass renderPass, 
	VkImageView depthImageView, 
	std::vector<VkFramebuffer>& swapchainFramebuffers )
{
	swapchainFramebuffers.resize ( vkDev.swapchainImageViews.size () );

	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		std::array<VkImageView, 2> attachments = {
			vkDev.swapchainImageViews[i],
			depthImageView
		};

		const VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = renderPass,
			.attachmentCount = static_cast<uint32_t>((depthImageView == VK_NULL_HANDLE) ? 1 : 2),
			.pAttachments = attachments.data (),
			.width = vkDev.framebufferWidth,
			.height = vkDev.framebufferHeight,
			.layers = 1
		};

		VK_CHECK ( vkCreateFramebuffer ( vkDev.device, &framebufferInfo, nullptr, &swapchainFramebuffers[i] ) );
	}

	return true;
}

bool createDepthOnlyFramebuffer ( 
	VulkanRenderDevice& vkDev, 
	uint32_t width, uint32_t height,
	VkRenderPass renderPass,
	VkImageView depthImageView, 
	VkFramebuffer* framebuffer )
{
	VkImageView attachment = depthImageView;

	const VkFramebufferCreateInfo framebufferInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = renderPass,
		.attachmentCount = 1u,
		.pAttachments = &attachment,
		.width = vkDev.framebufferWidth,
		.height = vkDev.framebufferHeight,
		.layers = 1
	};

	return (vkCreateFramebuffer ( vkDev.device, &framebufferInfo, nullptr, framebuffer ) == VK_SUCCESS);
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
		.viewType = viewType,
		.format = format,
		.subresourceRange = {
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};
	VK_CHECK ( vkCreateImageView ( device, &viewInfo, nullptr, imageView ) );
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
			indices.push_back ( mesh->mFaces[i].mIndices[j] );
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
	vkDestroyBuffer ( vkDev.device, stagingBuffer, nullptr );
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

std::string vulkanResultToString ( VkResult result )
{
	const std::map<VkResult, std::string> resultStrings = {
		{ VK_SUCCESS, "SUCCESS" },
		{ VK_NOT_READY, "NOT_READY" },
		{ VK_TIMEOUT, "TIMEOUT" },
		{ VK_EVENT_SET, "EVENT_SET" },
		{ VK_EVENT_RESET, "EVENT_RESET" },
		{ VK_INCOMPLETE, "INCOMPLETE" },
		{ VK_ERROR_OUT_OF_HOST_MEMORY, "ERROR_OUT_OF_HOST_MEMORY" },
		{ VK_ERROR_OUT_OF_DEVICE_MEMORY, "ERROR_OUT_OF_DEVICE_MEMORY" },
		{ VK_ERROR_INITIALIZATION_FAILED, "ERROR_INITIALIZATION_FAILED" },
		{ VK_ERROR_DEVICE_LOST, "ERROR_DEVICE_LOST" },
		{ VK_ERROR_MEMORY_MAP_FAILED, "ERROR_MEMORY_MAP_FAILED" },
		{ VK_ERROR_LAYER_NOT_PRESENT, "ERROR_LAYER_NOT_PRESENT" },
		{ VK_ERROR_EXTENSION_NOT_PRESENT, "ERROR_EXTENSION_NOT_PRESENT" },
		{ VK_ERROR_FEATURE_NOT_PRESENT, "ERROR_FEATURE_NOT_PRESENT" },
		{ VK_ERROR_INCOMPATIBLE_DRIVER, "ERROR_INCOMPATIBLE_DRIVER" },
		{ VK_ERROR_TOO_MANY_OBJECTS, "ERROR_TOO_MANY_OBJECTS" },
		{ VK_ERROR_FORMAT_NOT_SUPPORTED, "ERROR_FORMAT_NOT_SUPPORTED" },
		{ VK_ERROR_FRAGMENTED_POOL, "ERROR_FRAGMENTED_POOL" },
		{ VK_ERROR_UNKNOWN, "ERROR_UNKNOWN" },
		{ VK_ERROR_OUT_OF_POOL_MEMORY, "ERROR_OUT_OF_POOL_MEMORY" },
		{ VK_ERROR_INVALID_EXTERNAL_HANDLE, "ERROR_INVALID_EXTERNAL_HANDLE" },
		{ VK_ERROR_FRAGMENTATION, "ERROR_FRAGMENTATION" },
		{ VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" },
		{ VK_ERROR_SURFACE_LOST_KHR, "ERROR_SURFACE_LOST_KHR" },
		{ VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "ERROR_NATIVE_WINDOW_IN_USE_KHR" },
		{ VK_SUBOPTIMAL_KHR, "SUBOPTIMAL_KHR" },
		{ VK_ERROR_OUT_OF_DATE_KHR, "ERROR_OUT_OF_DATE_KHR" },
		{ VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "ERROR_INCOMPATIBLE_DISPLAY_KHR" },
		{ VK_ERROR_VALIDATION_FAILED_EXT, "ERROR_VALIDATION_FAILED_EXT" },
		{ VK_ERROR_INVALID_SHADER_NV, "ERROR_INVALID_SHADER_NV" },
		{ VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT, "ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT" },
		{ VK_ERROR_NOT_PERMITTED_EXT, "ERROR_NOT_PERMITTED_EXT" },
		{ VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, "ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" },
		{ VK_THREAD_IDLE_KHR, "THREAD_IDLE_KHR" },
		{ VK_THREAD_DONE_KHR, "THREAD_DONE_KHR" },
		{ VK_OPERATION_DEFERRED_KHR, "OPERATION_DEFERRED_KHR" },
		{ VK_OPERATION_NOT_DEFERRED_KHR, "OPERATION_NOT_DEFERRED_KHR" },
		{ VK_PIPELINE_COMPILE_REQUIRED_EXT, "PIPELINE_COMPILE_REQUIRED_EXT" },
		{ VK_ERROR_OUT_OF_POOL_MEMORY_KHR, "ERROR_OUT_OF_POOL_MEMORY_KHR" },
		{ VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR, "ERROR_INVALID_EXTERNAL_HANDLE_KHR" },
		{ VK_ERROR_FRAGMENTATION_EXT, "ERROR_FRAGMENTATION_EXT" },
		{ VK_ERROR_INVALID_DEVICE_ADDRESS_EXT, "ERROR_INVALID_DEVICE_ADDRESS_EXT" },
		{ VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR, "ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR" },
		{ VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT, "ERROR_PIPELINE_COMPILE_REQUIRED_EXT" },
		{ VK_RESULT_MAX_ENUM, "RESULT_MAX_ENUM" }
	};

	return resultStrings.find ( result )->second.c_str ();
}

void destroyVulkanImage ( VkDevice device, VulkanImage& image )
{
	vkDestroyImageView ( device, image.imageView, nullptr );
	vkDestroyImage ( device, image.image, nullptr );
	vkFreeMemory ( device, image.imageMemory, nullptr );
}

void destroyVulkanTexture ( VkDevice device, VulkanTexture& texture )
{
	destroyVulkanImage ( device, texture.image );
	vkDestroySampler ( device, texture.sampler, nullptr );
}

uint32_t bytesPerTexFormat ( VkFormat fmt )
{
	switch ( fmt )
	{
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_UNORM:
		return 1;
	case VK_FORMAT_R16_SFLOAT:
		return 2;
	case VK_FORMAT_R16G16_SFLOAT:
		return 4;
	case VK_FORMAT_R16G16_SNORM:
		return 4;
	case VK_FORMAT_B8G8R8A8_UNORM:
		return 4;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return 4;
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		return 4 * sizeof ( uint16_t );
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return 4 * sizeof ( float );
	default:
		break;
	}
	return 0;
}

bool updateTextureImage ( VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, const void* imageData, VkImageLayout sourceImageLayout )
{
	uint32_t bytesPerPixel = bytesPerTexFormat ( texFormat );

	VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
	VkDeviceSize imageSize = layerSize * layerCount;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer ( vkDev.device, vkDev.physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );

	uploadBufferData ( vkDev, stagingBufferMemory, 0, imageData, imageSize );

	transitionImageLayout ( vkDev, textureImage, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount );
	copyBufferToImage ( vkDev, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), layerCount );
	transitionImageLayout ( vkDev, textureImage, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount );

	vkDestroyBuffer ( vkDev.device, stagingBuffer, nullptr );
	vkFreeMemory ( vkDev.device, stagingBufferMemory, nullptr );

	return true;
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
	uint32_t width, uint32_t height, uint32_t layerCount )
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands ( vkDev );

	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = layerCount },
		.imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
		.imageExtent = VkExtent3D {.width = width, .height = height, .depth = 1 }
	};

	vkCmdCopyBufferToImage ( commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
	endSingleTimeCommands ( vkDev, commandBuffer );
}

void copyImageToBuffer ( VulkanRenderDevice& vkDev, VkImage image, VkBuffer buffer, uint32_t width, uint32_t height, uint32_t layerCount )
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands ( vkDev );

	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		},
		.imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
		.imageExtent = VkExtent3D {.width = width, .height = height, .depth = 1 }
	};

	vkCmdCopyImageToBuffer ( commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region );

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
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount }
	};

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if ( newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		(format == VK_FORMAT_D16_UNORM) ||
		(format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
		(format == VK_FORMAT_D32_SFLOAT) ||
		(format == VK_FORMAT_S8_UINT) ||
		(format == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(format == VK_FORMAT_D24_UNORM_S8_UINT) )
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if ( hasStencilComponent ( format ) )
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED )
	{
		switch ( newLayout )
		{
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case VK_IMAGE_LAYOUT_GENERAL:
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			/* Convert depth texture from undefined state to depth-stencil buffer */
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;
		default:
			break;
		}
	}
	else if ( oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
	{
		switch ( newLayout )
		{
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			/* Convert back from read-only to color attachment*/
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			/* Convert back from read-only to depth attachment */
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			/* Wait for render pass to complete */
			barrier.srcAccessMask = 0; // VK_ACCESS_SHADER_READ_BIT
			barrier.dstAccessMask = 0;

			/*
			* sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			* destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			* destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHEMTN_OUTPUT_BIT;
			*/

			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			/* Convert back from read-only to updateable */
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		default:
			break;
		}
	}
	else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
	{
		switch ( newLayout )
		{		
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			/* Convert from updateable texture to shader read-only */
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;	
		default:
			break;
		}
	}
	else if ( oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
	{
		switch ( newLayout )
		{
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			/* Convert from updateable texture to shader read-only */
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;	
		default:
			break;
		}

	}
	else if ( oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		switch ( newLayout )
		{
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			/* Convert from updateable depth texture to shader read-only */
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		default:
			break;
		}

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

bool executeComputeShader ( VulkanRenderDevice& vkDev, VkPipeline computePipeline, VkPipelineLayout pl, VkDescriptorSet ds, uint32_t xsize, uint32_t ysize, uint32_t zsize )
{
	/* The xSize, ySize, and zSize parameters are the numbers of local workgroups to dispatch in the X, Y, and Z dimensions. */
	// fill the command buffer
	VkCommandBuffer commandBuffer = vkDev.computeCommandBuffer;
	VkCommandBufferBeginInfo commandBufferBeginInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 0,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0
	};

	VK_CHECK ( vkBeginCommandBuffer ( commandBuffer, &commandBufferBeginInfo ) );

	/* To execute a compute shader, we should first bind the pipeline and the descriptor set object, and then emit the "vkCmdDispatch()" comand with the required execution range. */
	vkCmdBindPipeline ( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline );
	vkCmdBindDescriptorSets ( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pl, 0, 1, &ds, 0, 0 );
	vkCmdDispatch ( commandBuffer, xsize, ysize, zsize );

	/* Before the CPU can read back data written to a buffer by a compute shader, we have to insert a memory barrier. */
	VkMemoryBarrier readoutBarrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_HOST_READ_BIT
	};
	vkCmdPipelineBarrier ( commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &readoutBarrier, 0, nullptr, 0, nullptr );
	
	/* After adding all the commands, we complete the recording of the command buffer and submit to the queue. */
	VK_CHECK ( vkEndCommandBuffer ( commandBuffer ) );
	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO, 0, 0, 0, 0, 1, &commandBuffer, 0, 0
	};
	VK_CHECK ( vkQueueSubmit ( vkDev.computeQueue, 1, &submitInfo, 0 ) );

	/* To synchronize buffers between computations and rendering, we should wait for the compute shader completion. The simple way to do it is to just block until the GPU finishes its work. */
	VK_CHECK ( vkQueueWaitIdle ( vkDev.computeQueue ) );

	return true;
}

void insertComputedBufferBarrier ( VulkanRenderDevice& vkDev, VkCommandBuffer commandBuffer, VkBuffer buffer )
{
	uint32_t compute = vkDev.graphicsFamily;
	uint32_t graphics = vkDev.computeFamily;

	// make sure compute shader finishes before vertex shader reads vertices
	const VkBufferMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = compute,
		.dstQueueFamilyIndex = graphics,
		.buffer = buffer,
		.offset = 0,
		.size = VK_WHOLE_SIZE
	};

	vkCmdPipelineBarrier ( commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0 /*VK_FLAGS_NONE*/, 0, nullptr, 1, &barrier, 0, nullptr );
}

void insertComputedImageBarrier ( VkCommandBuffer commandBuffer, VkImage image )
{
	const VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
		.newLayout = VK_IMAGE_LAYOUT_GENERAL,
		.image = image,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } 
	};

	vkCmdPipelineBarrier ( commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0 /*VK_FLAGS_NONE*/, 0, nullptr, 0, nullptr, 1, &barrier);
}

std::vector<VkLayerProperties> getAvailableLayers ( void )
{
	uint32_t layerCount;
	std::vector<VkLayerProperties> availableLayers;

	vkEnumerateInstanceLayerProperties ( &layerCount, nullptr );

	availableLayers.resize ( static_cast<size_t>(layerCount) );

	vkEnumerateInstanceLayerProperties ( &layerCount, availableLayers.data () );

	return availableLayers;
}


std::vector<VkExtensionProperties> getAvailableExtensionsByLayer ( const char* layer )
{
	uint32_t extensionCount;
	std::vector<VkExtensionProperties> availableExtensions;

	vkEnumerateInstanceExtensionProperties ( layer, &extensionCount, nullptr );

	availableExtensions.resize ( static_cast<size_t>(extensionCount) );

	vkEnumerateInstanceExtensionProperties ( layer, &extensionCount, availableExtensions.data () );

	return availableExtensions;
}

std::vector<VkExtensionProperties> getAvailableExtensions ( void )
{
	uint32_t extensionCount;
	std::vector<VkExtensionProperties> availableExtensions;

	vkEnumerateInstanceExtensionProperties ( nullptr, &extensionCount, nullptr );

	availableExtensions.resize ( static_cast<size_t>(extensionCount) );

	vkEnumerateInstanceExtensionProperties ( nullptr, &extensionCount, availableExtensions.data () );

	return availableExtensions;
}

std::vector<const char*>	getGLFWRequiredExtensions ( void )
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions ( &glfwExtensionCount );

	std::vector<const char*> extensions ( glfwExtensions, glfwExtensions + glfwExtensionCount );

	return extensions;
}

bool checkValidationLayerSupport ( const std::vector<const char*>& requestedLayers )
{
	uint32_t layerCount;
	std::vector<VkLayerProperties> availableLayers;
	vkEnumerateInstanceLayerProperties ( &layerCount, nullptr );

	availableLayers.resize ( static_cast<size_t>(layerCount) );
	vkEnumerateInstanceLayerProperties ( &layerCount, availableLayers.data () );

	bool allLayersFound = false;

	for ( const char* layerName : requestedLayers )
	{
		bool layerFound = false;

		for ( const auto& layerProperties : availableLayers )
		{
			if ( strcmp ( layerName, layerProperties.layerName ) == 0 )
			{
				layerFound = true;
				break;
			}
		}

		if ( !layerFound )
		{
			return false;
		}
	}

	allLayersFound = true;
	return allLayersFound;
}

bool checkExtensionSupport ( const std::vector<const char*>& requestedExtensions )
{
	uint32_t extensionCount;
	std::vector<VkExtensionProperties> availableExtensions;

	vkEnumerateInstanceExtensionProperties ( nullptr, &extensionCount, nullptr );

	availableExtensions.resize ( static_cast<size_t>(extensionCount) );

	vkEnumerateInstanceExtensionProperties ( nullptr, &extensionCount, availableExtensions.data () );

	bool allExtensionsFound = false;

	for ( const char* extName : requestedExtensions )
	{
		bool extensionFound = false;

		for ( const auto& extProps : availableExtensions )
		{
			if ( strcmp ( extName, extProps.extensionName ) == 0 )
			{
				extensionFound = true;
				break;
			}
		}

		if ( !extensionFound )
		{
			return false;
		}
	}

	allExtensionsFound = true;
	return allExtensionsFound;
}

void printAvailableLayers ( void )
{
	uint32_t layerCount;
	std::vector<VkLayerProperties> availableLayers;

	vkEnumerateInstanceLayerProperties ( &layerCount, nullptr );

	availableLayers.resize ( static_cast<size_t>(layerCount) );

	vkEnumerateInstanceLayerProperties ( &layerCount, availableLayers.data () );

	cout << "----- AVAILABLE VULKAN LAYERS -----" << layerCount << endl;

	for ( const auto& layer : availableLayers )
	{
		cout << "==============================================================" << endl;
		cout << "NAME: " << layer.layerName << endl << "DESCRIPTION: " << layer.description << endl;
		cout << "==============================================================" << endl << endl;
	}

	cout << "----- DONE -----" << endl << endl;
}

void printAvailableExtensions ( void )
{
	uint32_t vulkanExtensionCount = 0;
	std::vector<VkExtensionProperties> extensions;
	vkEnumerateInstanceExtensionProperties ( nullptr, &vulkanExtensionCount, nullptr );

	extensions.resize ( vulkanExtensionCount );
	vkEnumerateInstanceExtensionProperties ( nullptr, &vulkanExtensionCount, extensions.data () );

	cout << "----- AVAILABLE VULKAN EXTENSIONS -----" << endl << endl;

	for ( const auto& ext : extensions )
	{
		cout << "==============================================================" << endl;
		cout << ext.extensionName << endl;
		cout << "==============================================================" << endl << endl;
	}

	cout << "----- DONE -----" << endl << endl;
}

void printExtensionsByLayer ( const char* layerName )
{
	uint32_t extensionCount;
	std::vector<VkExtensionProperties> availableExtensions;

	vkEnumerateInstanceExtensionProperties ( layerName, &extensionCount, nullptr );

	availableExtensions.resize ( static_cast<size_t>(extensionCount) );
	vkEnumerateInstanceExtensionProperties ( layerName, &extensionCount, availableExtensions.data () );

	cout << "----- AVAILABLE VULKAN EXTENSIONS FOR " << layerName << " LAYER -----" << endl << endl;

	for ( const auto& ext : availableExtensions )
	{
		cout << "==============================================================" << endl;
		cout << ext.extensionName << endl;
		cout << "==============================================================" << endl << endl;
	}

	cout << "----- DONE -----" << endl << endl;
}

void printAllExtensionsByLayer ( void )
{
	std::vector<VkLayerProperties> availableLayers = getAvailableLayers ();

	cout << "----- ENUMERATING VULKAN EXTENSIONS BY LAYER -----" << endl << endl;

	for ( const auto& layer : availableLayers )
	{
		cout << "==============================================================" << endl;
		cout << "- LAYER NAME: " << layer.layerName << endl << "  - LAYER DESCRIPTION: " << layer.description << endl << "  - AVAILABLE EXTENSIONS: " << endl;
		std::vector<VkExtensionProperties> availableExtensions = getAvailableExtensionsByLayer ( layer.layerName );
		for ( const auto& ext : availableExtensions )
		{
			cout << "    - " << ext.extensionName << endl;
		}
		cout << "==============================================================" << endl << endl;
	}
	cout << "----- DONE -----" << endl << endl;
}

void printVulkanApiVersion ( void )
{
	uint32_t apiVersion;
	VkResult apiCheckResult = vkEnumerateInstanceVersion ( &apiVersion );

	cout << "Result of vkEnumerateInstanceVersion: " << vulkanResultToString ( apiCheckResult ) << endl;

	cout << "VK_API_VERSION_MAJOR: " << VK_API_VERSION_MAJOR ( apiVersion ) << endl;
	cout << "VK_API_VERSION_MINOR: " << VK_API_VERSION_MINOR ( apiVersion ) << endl;
	cout << "VK_API_VERSION_VARIANT: " << VK_API_VERSION_VARIANT ( apiVersion ) << endl;
	cout << "VK_API_VERSION_PATCH: " << VK_API_VERSION_PATCH ( apiVersion ) << endl;
	cout << "VK_VERSION_MAJOR: " << VK_VERSION_MAJOR ( apiVersion ) << endl;
	cout << "VK_VERSION_MINOR: " << VK_VERSION_MINOR ( apiVersion ) << endl;
	cout << "VK_VERSION_PATCH: " << VK_VERSION_PATCH ( apiVersion ) << endl;
}
