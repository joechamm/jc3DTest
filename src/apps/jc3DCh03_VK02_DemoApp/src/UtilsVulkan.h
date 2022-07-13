#pragma once

#include <array>
#include <functional>
#include <vector>

#define VK_NO_PROTOTYPES
#include <Volk/volk.h>

#include <glslang/Include/glslang_c_interface.h>

static void VK_ASSERT ( bool check )
{
	if ( !check ) exit ( EXIT_FAILURE );
}

#define VK_CHECK(value) if (value != VK_SUCCESS) { VK_ASSERT(false); return false; }
#define VK_CHECK_RET(value) if (value != VK_SUCCESS) { VK_ASSERT(false); return value; }

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
	VkDebugReportCallbackEXT reportCallback;
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

struct VulkanTexture {
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
};

struct RenderPassCreateInfo final {
	bool clearColor_ = false;
	bool clearDepth_ = false;
	uint8_t flags_ = 0;
};

enum eRenderPassBit : uint8_t {  
	eRenderPassBit_First = 0x01,  // clear the attachment
	eRenderPassBit_Last = 0x02,		// transition to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 
	eRenderPassBit_Offscreen = 0x04, // transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMIAL
	eRenderPassBit_OffscreenInternal = 0x08 // keep VK_IMAGE_LAYOUT_*_ATTACHMENT_OPTIMAL
};

bool setupDebugCallbacks ( VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback );

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


void createInstance ( VkInstance* instance );

VkResult createDevice ( VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily, VkDevice* device );

VkResult findSuitablePhysicalDevice ( VkInstance instance, std::function<bool ( VkPhysicalDevice )> selector, VkPhysicalDevice* physicalDevice );

uint32_t findQueueFamilies ( VkPhysicalDevice device, VkQueueFlags desiredFlags );

VkFormat findSupportedFormat ( VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features );

VkFormat findDepthFormat ( VkPhysicalDevice device );

bool hasStencilComponent ( VkFormat format );

uint32_t findMemoryType ( VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties );

SwapchainSupportDetails querySwapchainSupport ( VkPhysicalDevice device, VkSurfaceKHR surface );

VkSurfaceFormatKHR chooseSwapSurfaceFormat ( const std::vector<VkSurfaceFormatKHR>& availableFormats );

VkPresentModeKHR chooseSwapPresentMode ( const std::vector<VkPresentModeKHR>& availablePresentModes );

uint32_t chooseSwapImageCount ( const VkSurfaceCapabilitiesKHR& caps );

VkResult createSwapchain ( VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t graphicsFamily, uint32_t width, uint32_t height, VkSwapchainKHR* swapchain );

size_t createSwapchainImages ( VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& swapchainImages, std::vector<VkImageView>& swapchainImageViews );

bool createDescriptorPool ( VkDevice device, uint32_t imageCount, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descPool );

bool createPipelineLayout ( VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout );

bool createColorAndDepthRenderPass ( VulkanRenderDevice& device, bool useDepth, VkRenderPass* renderPass, const RenderPassCreateInfo& ci, VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM );

bool createColorAndDepthFramebuffer ( VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView colorImageView, VkImageView depthImageView, VkFramebuffer* framebuffer );
bool createColorAndDepthFramebuffers ( VulkanRenderDevice& vkDev, VkRenderPass renderPass, VkImageView depthImageView, std::vector<VkFramebuffer>& swapchainFramebuffers );
bool createDepthOnlyFramebuffer ( VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView depthImageView, VkFramebuffer* framebuffer );

bool createGraphicsPipeline ( VkDevice device, uint32_t width, uint32_t height, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages, VkPipeline* pipeline );

bool createImageView ( VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1, uint32_t mipLevels = 1 );

bool createBuffer ( VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory );

bool createImage ( VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory );

void createDepthResources ( VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VulkanTexture& depth );

bool createTextureSampler ( VkDevice device, VkSampler* sampler );

bool createTextureImage ( VulkanRenderDevice& vkDev, const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory );

bool createTexturedVertexBuffer ( VulkanRenderDevice& vkDev, const char* filename, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t* vertexBufferSize, size_t* indexBufferSize );

bool setVkObjectName ( VulkanRenderDevice& vkDev, void* object, VkObjectType objType, const char* name );

VkResult createSemaphore ( VkDevice device, VkSemaphore* outSemaphore );

bool initVulkanRenderDevice ( VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool ( VkPhysicalDevice )> selector, VkPhysicalDeviceFeatures deviceFeatures );

void destroyVulkanRenderDevice ( VulkanRenderDevice& vkDev );

void destroyVulkanInstance ( VulkanInstance& vk );

void destroyVulkanTexture ( VkDevice device, VulkanTexture& texture );

bool isDeviceSuitable ( VkPhysicalDevice device );

VkCommandBuffer beginSingleTimeCommands ( VulkanRenderDevice& vkDev );
void endSingleTimeCommands ( VulkanRenderDevice& vkDev, VkCommandBuffer commandBuffer );
void copyBuffer ( VulkanRenderDevice& vkDev, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size );
void copyBufferToImage ( VulkanRenderDevice& vkDev, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height );
void transitionImageLayout ( VulkanRenderDevice& vkDev, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1, uint32_t mipLevels = 1 );
void transitionImageLayoutCmd ( VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1, uint32_t mipLevels = 1);

// begin/end SingleTimeCommands not using aggregate structs yet
// VkCommandBuffer beginSingleTimeCommands ( VkDevice device, VkCommandPool commandPool, VkQueue queue );

// void endSingleTimeCommands ( VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer );

// void copyBuffer ( VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size );

// void copyBufferToImage ( VkDevice device, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height );