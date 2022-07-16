#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <string>

#include "Utils.h"
#include "UtilsVulkan.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <string>

#include <helpers/RootDir.h>

#include <stdexcept>

using namespace std;

//using std::string;
using glm::mat4;
using glm::vec4;
using glm::vec3;

const uint32_t kScreenWidth = 1280;
const uint32_t kScreenHeight = 720;

const vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

GLFWwindow* window;

struct UniformBuffer {
	mat4 mvp;
} ubo;

static constexpr VkClearColorValue clearValueColor = { 1.0f, 1.0f, 1.0f, 1.0f };

size_t vertexBufferSize;
size_t indexBufferSize;

VulkanInstance vk;
VulkanRenderDevice vkDev;

struct VulkanState {
	// 1. Descriptor set (layout + pool + sets) -> uses uniform buffers, textures, framebuffers
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	// 2.
	std::vector<VkFramebuffer> swapchainFramebuffers;

	// 3. Pipeline &  render pass (using DescriptorSets & pipeline state options)
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	// 4. Uniform buffer
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	// 5. Storage Buffer with index and vertex data
	VkBuffer storageBuffer;
	VkDeviceMemory storageBufferMemory;

	// 6. Depth buffer
//	VulkanImage depthTexture;
	VulkanTexture depthTexture;

	VkSampler textureSampler;
//	VulkanImage texture;
	VulkanTexture texture;

	// try attaching shader modules to state
	ShaderModule vertShader;
	ShaderModule geomShader;
	ShaderModule fragShader;
} vkState;

bool createUniformBuffers ()
{
	VkDeviceSize bufferSize = sizeof ( UniformBuffer );
	vkState.uniformBuffers.resize ( vkDev.swapchainImages.size () );
	vkState.uniformBuffersMemory.resize ( vkDev.swapchainImages.size () );
	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		if ( !createBuffer (
			/* VkDevice device */					vkDev.device,
			/* VkPhysicalDevice physicalDevice */	vkDev.physicalDevice,
			/* VkDeviceSize size */					bufferSize,
			/* VkBufferUsageFlags usage */			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			/* VkMemoryPropertyFlags properties */	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			/* VkBuffer &buffer */					vkState.uniformBuffers[i],
			/* VkDeviceMemory &bufferMemory */		vkState.uniformBuffersMemory[i] ) )
		{
			printf ( "Fail: buffers\n" );
			return false;
		}		
	}

	return true;
}

void updateUniformBuffer ( uint32_t currentImage, const UniformBuffer& ubo, size_t uboSize )
{
	void* data = nullptr;
	vkMapMemory (
		/* VkDevice device */			vkDev.device, 
		/* VkDeviceMemory memory */		vkState.uniformBuffersMemory[currentImage], 
		/* VkDeviceSize offset */		0, 
		/* VkDeviceSize size */			uboSize, 
		/* VkMemoryMapFlags flags */	0, 
		/* void **ppData */				&data
	);
	memcpy ( data, &ubo, uboSize );
	vkUnmapMemory ( vkDev.device, vkState.uniformBuffersMemory[currentImage] );
}

bool fillCommandBuffers ( size_t i )
{
	const VkCommandBufferBeginInfo bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	const std::array<VkClearValue, 2> clearValues = {
		VkClearValue { .color = clearValueColor }, 
		VkClearValue { .depthStencil = { 1.0f, 0 } } };

	const VkRect2D screenRect = { 
		.offset = { 0, 0 }, 
		.extent = { .width = kScreenWidth, .height = kScreenHeight }
	};
	
	VK_CHECK ( vkBeginCommandBuffer ( vkDev.commandBuffers[i], &bi ) );
	
	const VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = vkState.renderPass,
		.framebuffer = vkState.swapchainFramebuffers[i],
		.renderArea = screenRect,
		.clearValueCount = static_cast<uint32_t>(clearValues.size ()),
		.pClearValues = clearValues.data ()
	};

	vkCmdBeginRenderPass (
		/* VkCommandBuffer commandBuffer */	vkDev.commandBuffers[i],
		/* const VkRenderPassBeginInfo *pRenderPassBegin */ &renderPassInfo,
		/* VkSubpassContents contents */ VK_SUBPASS_CONTENTS_INLINE 
	);
		
	vkCmdBindPipeline (
		/* VkCommandBuffer commandBuffer */				vkDev.commandBuffers[i],
		/* VkPipelineBindPoint pipelineBindPoint */		VK_PIPELINE_BIND_POINT_GRAPHICS,
		/* VkPipeline pipeline */						vkState.graphicsPipeline 
	);

	vkCmdBindDescriptorSets (
		/* VkCommandBuffer commandBuffer */				vkDev.commandBuffers[i],
		/* VkPipelineBindPoint pipelineBindPoint */		VK_PIPELINE_BIND_POINT_GRAPHICS,
		/* VkPipelineLayout layout */					vkState.pipelineLayout,
		/* uint32_t firstSet */							0,
		/* uint32_t descriptorSetCount */				1,
		/* const VkDescriptorSet *pDescriptorSets */	&vkState.descriptorSets[i],
		/* uint32_t dynamicOffsetCount */				0,
		/* const uint32_t *pDynamicOffsets */			nullptr
	);

	vkCmdDraw (
		/* VkCommandBuffer commandBuffer */				vkDev.commandBuffers[i],
		/* uint32_t vertexCount */						static_cast<uint32_t>(indexBufferSize / sizeof(uint32_t)),
		/* uint32_t instanceCount */					1,
		/* uint32_t firstVertex */						0,
		/* uint32_t firstInstance*/						0
	);

	vkCmdEndRenderPass ( vkDev.commandBuffers[i] );

	VK_CHECK ( vkEndCommandBuffer ( vkDev.commandBuffers[i] ) );
	return true;
}

// the descriptor set must have a fixed layout that describes the number and usage type of all the texture samples and buffers
bool createDescriptorSet ()
{
	// Declare a list of buffers and sampler descriptions. Each entry defines which shader unit it is bound to, the exact data type, and which shader stage can access it.
	const std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
		descriptorSetLayoutBinding ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )
	};

	const VkDescriptorSetLayoutCreateInfo li = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size ()),
		.pBindings = bindings.data ()
	};

	VK_CHECK ( vkCreateDescriptorSetLayout ( vkDev.device, &li, nullptr, &vkState.descriptorSetLayout ) );

	// allocate a number of descriptor set layouts, one for each swap chain image, just like we did with the uniform and command buffers
	std::vector<VkDescriptorSetLayout> layouts ( vkDev.swapchainImages.size (), vkState.descriptorSetLayout );
	VkDescriptorSetAllocateInfo ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = vkState.descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(vkDev.swapchainImages.size ()),
		.pSetLayouts = layouts.data ()
	};
	vkState.descriptorSets.resize ( vkDev.swapchainImages.size () );
	VK_CHECK ( vkAllocateDescriptorSets ( vkDev.device, &ai, vkState.descriptorSets.data () ) );

	// Once we have allocated the descriptor sets with the specified layout, we must update these descriptor sets with concreate buffer and texture handles.
	// This operation can be viewed as an analogue of texture and buffer binding in OpenGL. The crucial difference is that we do not do this at every frame since
	// binding is prebaked into the pipeline. The minor downside of this approach is that we cannot simply change the texture from frame to frame.
	// For this example we will use one uniform buffer, one index buffer, one vertex buffer, and one texture
	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		VkDescriptorBufferInfo bufferInfo = {
			.buffer = vkState.uniformBuffers[i],
			.offset = 0,
			.range = sizeof ( UniformBuffer )
		};

		VkDescriptorBufferInfo bufferInfo2 = {
			.buffer = vkState.storageBuffer,
			.offset = 0,
			.range = vertexBufferSize
		};
		VkDescriptorBufferInfo bufferInfo3 = {
			.buffer = vkState.storageBuffer,
			.offset = vertexBufferSize,
			.range = indexBufferSize
		};
		VkDescriptorImageInfo imageInfo = {
			.sampler = vkState.textureSampler,
			.imageView = vkState.texture.image.imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		// The VkWriteDescriptorSet operation array contains all the "bindings" for the buffers we declared previously
		std::array<VkWriteDescriptorSet, 4> descriptorWrites = {
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vkState.descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &bufferInfo
				},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vkState.descriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &bufferInfo2
				},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vkState.descriptorSets[i],
				.dstBinding = 2,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &bufferInfo3
				},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vkState.descriptorSets[i],
				.dstBinding = 3,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfo
				},
		};

		// update the descriptor by applying the necessary descripor write operations
		vkUpdateDescriptorSets ( vkDev.device, static_cast<uint32_t>(descriptorWrites.size ()), descriptorWrites.data (), 0, nullptr );
	}

	return true;
}

bool checkValidationLayerSupport ()
{
	uint32_t layerCount;
	vector<VkLayerProperties> availableLayers;
	vkEnumerateInstanceLayerProperties ( &layerCount, nullptr );

	availableLayers.resize ( static_cast<size_t>(layerCount) );
	vkEnumerateInstanceLayerProperties ( &layerCount, availableLayers.data () );

	for ( const char* layerName : validationLayers )
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

	return true;
}

vector<const char*> getRequiredExtensions ()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions ( &glfwExtensionCount );

	vector<const char*> extensions ( glfwExtensions, glfwExtensions + glfwExtensionCount );

	if ( enableValidationLayers )
	{
		extensions.push_back ( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
//		extensions.push_back ( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
	}

	return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (
	VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
	void* UserData )
{
	printf ( "Validation layer: %s\n", CallbackData->pMessage );
	return VK_FALSE;
}

//static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback (
//	VkDebugReportFlagsEXT flags,
//	VkDebugReportObjectTypeEXT objectType,
//	uint64_t object,
//	size_t location,
//	int32_t messageCode,
//	const char* pLayerPrefix,
//	const char* pMessage,
//	void* UserData )
//{
//	// https://github.com/zeux/niagara/blob/master/src/device.cpp   [ignoring performance warnings]
//	// This silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
//	if ( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT )
//	{
//		return VK_FALSE;
//	}
//
//	printf ( "Debug callback (%s): %s\n", pLayerPrefix, pMessage );
//	return VK_FALSE;
//}

void populateDebugMessengerCreateInfo ( VkDebugUtilsMessengerCreateInfoEXT& createInfo )
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

//void populateDebugReportCallbackCreateInfo ( VkDebugReportCallbackCreateInfoEXT& createInfo )
//{
//	createInfo = {};
//	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
//	createInfo.pNext = nullptr;
//	createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
//		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
//		VK_DEBUG_REPORT_ERROR_BIT_EXT |
//		VK_DEBUG_REPORT_DEBUG_BIT_EXT;
//
//	createInfo.pfnCallback = debugReportCallback;
//	createInfo.pUserData = nullptr;
//}

void altCreateInstance ( VkInstance* instance )
{
	if ( enableValidationLayers && !checkValidationLayerSupport () )
	{
		throw runtime_error ( "validation layers requested, but not available!" );
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "jc3DCh03_VK02_DemoApp";
	appInfo.applicationVersion = VK_MAKE_VERSION ( 1, 0, 0 );
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION ( 1, 0, 0 );
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions ();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size ());
	createInfo.ppEnabledExtensionNames = extensions.data ();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
//	VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = {};
	if ( enableValidationLayers )
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size ());
		createInfo.ppEnabledLayerNames = validationLayers.data ();

		populateDebugMessengerCreateInfo ( debugCreateInfo );
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
//		populateDebugReportCallbackCreateInfo ( debugReportCreateInfo );
	} else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if ( vkCreateInstance ( &createInfo, nullptr, instance ) != VK_SUCCESS )
	{
		throw runtime_error ( "failed to create instance!" );
	}

	volkLoadInstance ( *instance );
}

vector<VkLayerProperties> getAvailableLayers ( void )
{
	uint32_t layerCount;
	vector<VkLayerProperties> availableLayers;

	vkEnumerateInstanceLayerProperties ( &layerCount, nullptr );

	availableLayers.resize ( static_cast<size_t>(layerCount) );

	vkEnumerateInstanceLayerProperties ( &layerCount, availableLayers.data () );

	return availableLayers;
}

vector<VkExtensionProperties> getAvailableExtensionsByLayer ( const char* layer )
{
	uint32_t extensionCount;
	vector<VkExtensionProperties> availableExtensions;

	vkEnumerateInstanceExtensionProperties ( layer, &extensionCount, nullptr );

	availableExtensions.resize ( static_cast<size_t>(extensionCount) );

	vkEnumerateInstanceExtensionProperties ( layer, &extensionCount, availableExtensions.data () );

	return availableExtensions;
}

void enumExtensions ( void )
{
	uint32_t vulkanExtensionCount = 0;
	vector<VkExtensionProperties> extensions;
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

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions ( &glfwExtensionCount );

	cout << "----- GLFW REQUIRED EXTENSIONS -----" << endl << endl;

	for ( uint32_t i = 0; i < glfwExtensionCount; i++ )
	{
		cout << "==============================================================" << endl;
		cout << glfwExtensions[i] << endl;
		cout << "==============================================================" << endl << endl;
	}

	cout << "----- DONE -----" << endl << endl;
}

void enumExtensonsByLayer ( const char* layerName )
{
	uint32_t extensionCount;
	vector<VkExtensionProperties> availableExtensions;

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

void enumLayers ( void )
{
	uint32_t layerCount;
	vector<VkLayerProperties> availableLayers;

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

void enumAllExtensionsByLayers ( void )
{
	vector<VkLayerProperties> availableLayers = getAvailableLayers ();

	cout << "----- ENUMERATING VULKAN EXTENSIONS BY LAYER -----" << endl << endl;

	for ( const auto& layer : availableLayers )
	{
		cout << "==============================================================" << endl;
		cout << "- LAYER NAME: " << layer.layerName << endl << "  - LAYER DESCRIPTION: " << layer.description << endl << "  - AVAILABLE EXTENSIONS: " << endl;
		vector<VkExtensionProperties> availableExtensions = getAvailableExtensionsByLayer ( layer.layerName );
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
	VkResult result = vkEnumerateInstanceVersion ( &apiVersion );

	cout << "Result of vkEnumerateInstanceVersion: " << vulkanResultToString ( result ) << endl;

	cout << "VK_API_VERSION_MAJOR: " << VK_API_VERSION_MAJOR ( apiVersion ) << endl;
	cout << "VK_API_VERSION_MINOR: " << VK_API_VERSION_MINOR ( apiVersion ) << endl;
	cout << "VK_API_VERSION_VARIANT: " << VK_API_VERSION_VARIANT ( apiVersion ) << endl;
	cout << "VK_API_VERSION_PATCH: " << VK_API_VERSION_PATCH ( apiVersion ) << endl;
	cout << "VK_VERSION_MAJOR: " << VK_VERSION_MAJOR ( apiVersion ) << endl;
	cout << "VK_VERSION_MINOR: " << VK_VERSION_MINOR ( apiVersion ) << endl;
	cout << "VK_VERSION_PATCH: " << VK_VERSION_PATCH ( apiVersion ) << endl;
}

bool initVulkan ()
{
	// Debugging
//	enumLayers ();
//	enumExtensions ();

	printVulkanApiVersion ();

	enumAllExtensionsByLayers ();

	// creat vulkan instance and set up debugging callbacks
//	createInstance ( &vk.instance );

	cout << "Attempting to create Vulkan Instance..." << endl;

	altCreateInstance ( &vk.instance );

	cout << "Attempting to setup debug callbacks..." << endl;

	if ( !setupDebugCallbacks ( vk.instance, &vk.messenger ) )
	{
		exit ( EXIT_FAILURE );
	}

	cout << "Attempting to create window surface..." << endl;
	// create a window surface attached to the GLFW window and our Vulkan instance
	if ( glfwCreateWindowSurface ( vk.instance, window, nullptr, &vk.surface ) )
	{
		exit ( EXIT_FAILURE );
	}

	cout << "Attempting to initialize vulkan render device..." << endl;

	// initialize Vulkan objects
	if ( !initVulkanRenderDevice ( vk, vkDev, kScreenWidth, kScreenHeight, isDeviceSuitable, { .geometryShader = VK_TRUE } ) )
	{
		exit ( EXIT_FAILURE );
	}

	cout << "Attempting to create textured vertex buffer..." << endl;

	// createTexturedVertexBuffer is failing because the graphics queue hasn't been created yet
	if ( !createTexturedVertexBuffer ( vkDev, string ( ROOT_DIR ).append ( "assets/models/rubber_duck/scene.gltf" ).c_str (), &vkState.storageBuffer, &vkState.storageBufferMemory, &vertexBufferSize, &indexBufferSize ) || !createUniformBuffers () )
	{
		printf ( "Cannot create data buffers\n" );
		fflush ( stdout );
		exit ( 1 );
	}

	cout << "Attempting to create texture image..." << endl;

	createTextureImage ( vkDev, string ( ROOT_DIR ).append ( "assets/models/rubber_duck/textures/Duck_baseColor.png").c_str(), vkState.texture.image.image, vkState.texture.image.imageMemory );

	cout << "Attempting to create image view..." << endl;

	createImageView ( vkDev.device, vkState.texture.image.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &vkState.texture.image.imageView );
	
	cout << "Attempting to create texture sampler..." << endl;

	createTextureSampler ( vkDev.device, &vkState.textureSampler );

	cout << "Attempting to create depth resources..." << endl;

	createDepthResources ( vkDev, kScreenWidth, kScreenHeight, vkState.depthTexture.image );

	cout << "Attempting to create pipeline..." << endl;

	string vsFilename = string ( ROOT_DIR ) + string ( "assets/shaders/VK02.vert" );
	string fsFilename = string ( ROOT_DIR ) + string ( "assets/shaders/VK02.frag" );
	string gsFilename = string ( ROOT_DIR ) + string ( "assets/shaders/VK02.geom" );

	//const std::vector<const char*> shaderFiles = {
	//	string ( ROOT_DIR ).append ( "assets/shaders/VK02.vert" ).c_str (),
	//	string ( ROOT_DIR ).append ( "assets/shaders/VK02.frag" ).c_str (),
	//	string ( ROOT_DIR ).append ( "assets/shaders/VK02.geom" ).c_str ()
	//};

	vector<const char*> shaderFiles = {
		vsFilename.c_str (),
		fsFilename.c_str (),
		gsFilename.c_str ()
	};

	cout << "shader filenames: " << endl;
	for ( const auto& fname : shaderFiles )
	{
		cout << fname << endl;
	}

	//if ( !createDescriptorPool ( vkDev, 1, 2, 1, &vkState.descriptorPool ) ||
	//	!createDescriptorSet () ||
	//	!createColorAndDepthRenderPass ( vkDev, true, &vkState.renderPass, RenderPassCreateInfo{ .clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First | eRenderPassBit_Last } ) ||
	//	!createPipelineLayout ( vkDev.device, vkState.descriptorSetLayout, &vkState.pipelineLayout ) ||
	//	!createGraphicsPipeline ( vkDev, vkState.renderPass, vkState.pipelineLayout, shaderFiles, &vkState.graphicsPipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true, false, -1, -1, 0) )
	//{
	//	printf ( "Failed to create pipeline\n" );
	//	fflush ( stdout );
	//	exit ( 0 );
	//}

	cout << "creating descriptor pool..." << endl;
	if ( !createDescriptorPool ( vkDev, 1, 2, 1, &vkState.descriptorPool ) )
	{
		cout << "failed to create descriptor pool..." << endl;
		fflush ( stdout );
		exit ( 0 );
	}

	cout << "creating descriptor set..." << endl;
	if ( !createDescriptorSet() )
	{
		cout << "failed to create descriptor set..." << endl;
		fflush ( stdout );
		exit ( 0 );
	}

	cout << "creating color and depth render pass..." << endl;
	const RenderPassCreateInfo rpci = {
		.clearColor_ = true,
		.clearDepth_ = true,
		.flags_ = eRenderPassBit_First | eRenderPassBit_Last
	};

	if ( !createColorAndDepthRenderPass(vkDev, true, &vkState.renderPass, rpci ) )
	{
		cout << "failed to create render pass..." << endl;
		fflush ( stdout );
		exit ( 0 );
	}

	cout << "creating pipeline layout..." << endl;
	if ( !createPipelineLayout ( vkDev.device, vkState.descriptorSetLayout, &vkState.pipelineLayout ) )
	{
		cout << "failed to create pipeline layout..." << endl;
		fflush ( stdout );
		exit ( 0 );
	}

	cout << "creating graphics pipeline..." << endl;
	if ( !createGraphicsPipeline ( vkDev, vkState.renderPass, vkState.pipelineLayout, shaderFiles, &vkState.graphicsPipeline ) )
	{
		cout << "failed to create graphics pipeline..." << endl;
		fflush ( stdout );
		exit ( 0 );
	}

	cout << "attempting to create color and depth framebuffers..." << endl;
	if ( !createColorAndDepthFramebuffers ( vkDev, vkState.renderPass, vkState.depthTexture.image.imageView, vkState.swapchainFramebuffers ) )
	{
		cout << "failed to create framebuffers..." << endl;
		fflush ( stdout );
		exit ( 0 );
	}

	return VK_SUCCESS;
}

void terminateVulkan ()
{
	vkDestroyBuffer ( vkDev.device, vkState.storageBuffer, nullptr );
	vkFreeMemory ( vkDev.device, vkState.storageBufferMemory, nullptr );

	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		vkDestroyBuffer ( vkDev.device, vkState.uniformBuffers[i], nullptr );
		vkFreeMemory ( vkDev.device, vkState.uniformBuffersMemory[i], nullptr );
	}

	vkDestroyDescriptorSetLayout ( vkDev.device, vkState.descriptorSetLayout, nullptr );
	vkDestroyDescriptorPool ( vkDev.device, vkState.descriptorPool, nullptr );

	for ( auto framebuffer : vkState.swapchainFramebuffers )
	{
		vkDestroyFramebuffer ( vkDev.device, framebuffer, nullptr );
	}

	vkDestroySampler ( vkDev.device, vkState.textureSampler, nullptr );
	destroyVulkanTexture ( vkDev.device, vkState.texture );
	destroyVulkanTexture ( vkDev.device, vkState.depthTexture );
	
	vkDestroyRenderPass ( vkDev.device, vkState.renderPass, nullptr );

	vkDestroyPipelineLayout ( vkDev.device, vkState.pipelineLayout, nullptr );
	vkDestroyPipeline ( vkDev.device, vkState.graphicsPipeline, nullptr );

	destroyVulkanRenderDevice ( vkDev );

	destroyVulkanInstance ( vk );
}

bool drawOverlay ()
{
	// aquire the next available image from the swap chain and reset the command pool
	uint32_t imageIndex = 0;
	if ( vkAcquireNextImageKHR ( vkDev.device, vkDev.swapchain, 0, vkDev.semaphore, VK_NULL_HANDLE, &imageIndex ) != VK_SUCCESS )
	{
		return false;
	}

	VK_CHECK ( vkResetCommandPool ( vkDev.device, vkDev.commandPool, 0 ) );

	// fill in the uniform buffer with data. rotate the model around the vertical axis
	int width, height;
	glfwGetFramebufferSize ( window, &width, &height );
	const float ratio = width / (float)height;

	const mat4 m1 = glm::rotate ( glm::translate ( mat4 ( 1.0f ), vec3 ( 0.0f, 0.5f, -1.5f ) ) * glm::rotate ( mat4 ( 1.0f ), glm::pi<float> (), vec3 ( 1, 0, 0 ) ), (float)glfwGetTime (), vec3 ( 0.0f, 1.0f, 0.0f ) );
	const mat4 p = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );

	const UniformBuffer ubo{ .mvp = p * m1 };

	updateUniformBuffer ( imageIndex, ubo, sizeof(ubo) );

	// fill the command buffers. not necessary in this recipe since the commands are identical
	fillCommandBuffers (imageIndex);

	const VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

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

	VK_CHECK ( vkQueueSubmit ( vkDev.graphicsQueue, 1, &si, nullptr ) );

	const VkPresentInfoKHR pi = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &vkDev.swapchain,
		.pImageIndices = &imageIndex
	};

	VK_CHECK ( vkQueuePresentKHR ( vkDev.graphicsQueue, &pi ) );
	VK_CHECK ( vkDeviceWaitIdle ( vkDev.device ) );
	return true;
}

int main ()
{
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
	window = glfwCreateWindow ( kScreenWidth, kScreenHeight, "Vulkan Demo App", nullptr, nullptr );

	if ( !window )
	{
		glfwTerminate ();
		exit ( EXIT_FAILURE );
	}

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

	while ( !glfwWindowShouldClose ( window ) )
	{
		drawOverlay ();
		glfwPollEvents ();
	}

	terminateVulkan ();
	glfwTerminate ();
	glslang_finalize_process ();

	return 0;
}