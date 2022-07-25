#pragma once

#include <array>
#include <functional>
#include <vector>
#include <map>
#include <string>

#define VK_NO_PROTOTYPES
#include <volk/volk.h>

#include "glslang_c_interface.h"

#define VK_CHECK(value) CHECK(value == VK_SUCCESS, __FILE__, __LINE__);
#define VK_CHECK_RET(value) if (value != VK_SUCCESS) { CHECK(false, __FILE__,__LINE__); return value; }
#define BL_CHECK(value) CHECK(value, __FILE__,__LINE__);

struct ShaderModule final {
	std::vector<unsigned int> SPIRV;
	VkShaderModule shaderModule;
};

struct SwapchainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities = {};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct VulkanInstance {
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT messenger;
};

struct VulkanRenderDevice {

	uint32_t framebufferWidth;
	uint32_t framebufferHeight;

	VkDevice device;
	VkQueue graphicsQueue;
	VkPhysicalDevice physicalDevice;
	
	uint32_t graphicsFamily;
	
	VkSemaphore semaphore;
	VkSemaphore renderSemaphore;
	VkSwapchainKHR swapchain;
	
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
};

struct VulkanBuffer {
	VkBuffer buffer;
	VkDeviceSize size;
	VkDeviceMemory memory;

	/* Permanent mapping to CPU address space (see VulkanResources::addBuffer) */
	void* ptr;
};

struct VulkanImage {
	VkImage image = nullptr;
	VkDeviceMemory imageMemory = nullptr;
	VkImageView imageView = nullptr;
};

// Aggregate structure for passing around the texture data
struct VulkanTexture {

	uint32_t width;
	uint32_t height;
	uint32_t depth;
	VkFormat format;

	VulkanImage image;
	VkSampler sampler;

	// Offscreen buffers require VK_IMAGE_LAYOUT_GENERAL && static textures have VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	VkImageLayout desiredLayout;
};

void CHECK ( bool check, const char* filename, int linenumber );

bool setupDebugCallbacks ( VkInstance instance, VkDebugUtilsMessengerEXT* messenger );

VkResult createShaderModule ( VkDevice device, ShaderModule* shader, const char* fileName );

size_t compileShaderFile ( const char* file, ShaderModule& shaderModule );

/* shaderStageInfo isn't in the book just github code, but is referenced in it */
inline VkPipelineShaderStageCreateInfo shaderStageInfo ( VkShaderStageFlagBits shaderStage, ShaderModule& module, const char* entryPoint )
{
	return VkPipelineShaderStageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = shaderStage,
		.module = module.shaderModule,
		.pName = entryPoint,
		.pSpecializationInfo = nullptr
	};
}

VkShaderStageFlagBits glslangShaderStageToVulkan ( glslang_stage_t sh );

/* descriptorSetLayoutBinding doesn't seem to be in the book, just the github code */
inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding ( uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1 )
{
	return VkDescriptorSetLayoutBinding{
		.binding = binding,
		.descriptorType = descriptorType,
		.descriptorCount = descriptorCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}

inline VkWriteDescriptorSet bufferWriteDescriptorSet ( VkDescriptorSet ds, const VkDescriptorBufferInfo* bi, uint32_t bindIdx, VkDescriptorType dType )
{
	return VkWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = ds,
		.dstBinding = bindIdx,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = dType,
		.pImageInfo = nullptr,
		.pBufferInfo = bi,
		.pTexelBufferView = nullptr
	};
}

inline VkWriteDescriptorSet imageWriteDescriptorSet ( VkDescriptorSet ds, const VkDescriptorImageInfo* ii, uint32_t bindIndex )
{
	return VkWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = ds,
		.dstBinding = bindIndex,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = ii,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr
	};
}

void createInstance ( VkInstance* instance );
void createInstanceWithDebugging ( VkInstance* instance, const char* appName = "jc3DTest Vulkan Application" );

VkResult createDevice ( VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily, VkDevice* device );

VkResult createSwapchain ( VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t graphicsFamily, uint32_t width, uint32_t height, VkSwapchainKHR* swapchain );

size_t createSwapchainImages ( VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& swapchainImages, std::vector<VkImageView>& swapchainImageViews );

VkResult createSemaphore ( VkDevice device, VkSemaphore* outSemaphore );

bool createTextureSampler ( VkDevice device, VkSampler* sampler );

//bool createDescriptorPool ( VkDevice device, uint32_t imageCount, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descPool );
bool createDescriptorPool ( VulkanRenderDevice& vkDev, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descriptorPool );

bool isDeviceSuitable ( VkPhysicalDevice device );

SwapchainSupportDetails querySwapchainSupport ( VkPhysicalDevice device, VkSurfaceKHR surface );

VkSurfaceFormatKHR chooseSwapSurfaceFormat ( const std::vector<VkSurfaceFormatKHR>& availableFormats );

VkPresentModeKHR chooseSwapPresentMode ( const std::vector<VkPresentModeKHR>& availablePresentModes );

uint32_t chooseSwapImageCount ( const VkSurfaceCapabilitiesKHR& caps );

VkResult findSuitablePhysicalDevice ( VkInstance instance, std::function<bool ( VkPhysicalDevice )> selector, VkPhysicalDevice* physicalDevice );

uint32_t findQueueFamilies ( VkPhysicalDevice device, VkQueueFlags desiredFlags );

VkFormat findSupportedFormat ( VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features );

uint32_t findMemoryType ( VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties );

VkFormat findDepthFormat ( VkPhysicalDevice device );

bool hasStencilComponent ( VkFormat format );

bool createGraphicsPipeline (
	VulkanRenderDevice& vkDev,
	VkRenderPass renderPass, 
	VkPipelineLayout pipelineLayout,
	const std::vector<const char*>& shaderFiles,
	VkPipeline* pipeline,
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST /* defaults to triangles */,
	bool useDepth = true,
	bool useBlending = true,
	bool dynamicScissorState = false,
	int32_t customWidth = -1,
	int32_t customHeight = -1,
	uint32_t numPatchControlPoints = 0 );

bool createGraphicsPipeline (
	VulkanRenderDevice& vkDev,
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<std::string>& shaderFiles,
	VkPipeline* pipeline,
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	bool useDepth = true,
	bool useBlending = true,
	bool dynamicScissorState = false,
	int32_t customWidth = -1,
	int32_t customHeight = -1,
	uint32_t numPatchControlPoints = 0 );

//bool createGraphicsPipeline ( VkDevice device, uint32_t width, uint32_t height, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages, VkPipeline* pipeline );

bool createBuffer ( VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory );

//bool createImage ( VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory );
bool createImage ( VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageCreateFlags flags = 0, uint32_t mipLevels = 1 );

bool createUniformBuffer ( VulkanRenderDevice& vkDev, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize );

/* Copy [data] to GPU device buffer */
void uploadBufferData ( VulkanRenderDevice& vkDev, const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, const void* data, const size_t dataSize );

bool createImageView ( VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1, uint32_t mipLevels = 1 );

enum eRenderPassBit : uint8_t {
	eRenderPassBit_First = 0x01,  // clear the attachment
	eRenderPassBit_Last = 0x02,		// transition to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 
	eRenderPassBit_Offscreen = 0x04, // transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMIAL
	eRenderPassBit_OffscreenInternal = 0x08 // keep VK_IMAGE_LAYOUT_*_ATTACHMENT_OPTIMAL
};

struct RenderPassCreateInfo final {
	bool clearColor_ = false;
	bool clearDepth_ = false;
	uint8_t flags_ = 0;
};

bool createColorAndDepthRenderPass ( VulkanRenderDevice& device, bool useDepth, VkRenderPass* renderPass, const RenderPassCreateInfo& ci, VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM );

VkCommandBuffer beginSingleTimeCommands ( VulkanRenderDevice& vkDev );
void endSingleTimeCommands ( VulkanRenderDevice& vkDev, VkCommandBuffer commandBuffer );
void copyBuffer ( VulkanRenderDevice& vkDev, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size );
void transitionImageLayout ( VulkanRenderDevice& vkDev, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1, uint32_t mipLevels = 1 );
void transitionImageLayoutCmd ( VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1, uint32_t mipLevels = 1 );

bool initVulkanRenderDevice ( VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool ( VkPhysicalDevice )> selector, VkPhysicalDeviceFeatures deviceFeatures );
void destroyVulkanRenderDevice ( VulkanRenderDevice& vkDev );
void destroyVulkanInstance ( VulkanInstance& vk );

bool createColorAndDepthFramebuffer ( VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView colorImageView, VkImageView depthImageView, VkFramebuffer* framebuffer );
bool createColorAndDepthFramebuffers ( VulkanRenderDevice& vkDev, VkRenderPass renderPass, VkImageView depthImageView, std::vector<VkFramebuffer>& swapchainFramebuffers );
bool createDepthOnlyFramebuffer ( VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView depthImageView, VkFramebuffer* framebuffer );

void copyBufferToImage ( VulkanRenderDevice& vkDev, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount = 1 );

void destroyVulkanImage ( VkDevice device, VulkanImage& image );
void destroyVulkanTexture ( VkDevice device, VulkanTexture& texture );

uint32_t bytesPerTexFormat ( VkFormat fmt );

/* VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for real update of an existing texture */
bool updateTextureImage ( VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, const void* imageData, VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED );

bool createDepthResources ( VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VulkanImage& depth );

bool createPipelineLayout ( VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout );

bool createTextureImageFromData ( VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory, void* imageData, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount = 1, VkImageCreateFlags flags = 0 );

bool createTextureImage ( VulkanRenderDevice& vkDev, const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory );

bool createCubeTextureImage ( VulkanRenderDevice& vkDev, const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width = nullptr, uint32_t* height = nullptr );

bool createTexturedVertexBuffer ( VulkanRenderDevice& vkDev, const char* filename, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t* vertexBufferSize, size_t* indexBufferSize );

bool setVkObjectName ( VulkanRenderDevice& vkDev, void* object, VkObjectType objType, const char* name );

inline bool setVkImageName ( VulkanRenderDevice& vkDev, void* object, const char* name )
{
	return setVkObjectName ( vkDev, object, VK_OBJECT_TYPE_IMAGE, name );
}

/* Added for debuggging */

std::string vulkanResultToString ( VkResult result );

std::vector<VkLayerProperties> getAvailableLayers ( void );
std::vector<VkExtensionProperties> getAvailableExtensionsByLayer ( const char* layer );
std::vector<VkExtensionProperties> getAvailableExtensions ( void );
std::vector<const char*>	getGLFWRequiredExtensions ( void );
void printAvailableLayers ( void );
void printAvailableExtensions ( void );
void printExtensionsByLayer ( const char* layerName );
void printAllExtensionsByLayer ( void );
void printVulkanApiVersion ( void );
bool checkValidationLayerSupport ( const std::vector<const char*>& requestedLayers );
bool checkExtensionSupport ( const std::vector<const char*>& requestedExtensions );
void populateDebugMessengerCreateInfo ( VkDebugUtilsMessengerCreateInfoEXT& createInfo );
