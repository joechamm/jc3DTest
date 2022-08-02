#ifndef __VULKAN_RESOURCES_H__
#define __VULKAN_RESOURCES_H__

#include <jcVkFramework/UtilsVulkan.h>
#include <volk/volk.h>

#include <cstring>
#include <memory>
#include <map>
#include <utility>

struct VulkanResources
{
private:
	VulkanRenderDevice& vkDev_;
	std::vector<VulkanTexture> allTextures_;
	std::vector<VulkanBuffer> allBuffers_;
	std::vector<VkFramebuffer> allFramebuffers_;
	std::vector<VkRenderPass> allRenderpasses_;
	std::vector<VkPipelineLayout> allPipelineLayouts_;
	std::vector<VkPipeline> allPipelines_;
	std::vector<VkDescriptorSetLayout> allDSLayouts_;
	std::vector<VkDescriptorPool> allDPools_;

	std::vector<ShaderModule> shaderModules_;
	std::map<std::string, int> shaderMap_;

public:
	explicit VulkanResources ( VulkanRenderDevice& vkDev ) : vkDev_ ( vkDev )
	{}

	~VulkanResources ();
	
	VulkanTexture loadTexture2D ( const char* filename );
	VulkanTexture loadCubeMap ( const char* filename, uint32_t mipLevels = 1);
	VulkanTexture loadKTX ( const char* filename );
	VulkanTexture createFontTexture ( const char* fontfile );

	VulkanTexture addColorTexture ( int texWidth = 0, int texHeight = 0, VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM, VkFilter minFilter = VK_FILTER_LINEAR, VkFilter maxFilter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT );
	VulkanTexture addDepthTexture ( int texWidth = 0, int texHeight = 0, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );

	VulkanBuffer addBuffer ( VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool createMapping = false );

	VkFramebuffer addFramebuffer ( RenderPass renderPass, const std::vector<VulkanTexture>& images );

	RenderPass addRenderPass ( const std::vector<VulkanTexture>& outputs, const RenderPassCreateInfo& ci = { .clearColor_ = false, .clearDepth_ = true, .flags_ = eRenderPassBit_Offscreen | eRenderPassBit_First }, bool useDepth = true );
	RenderPass addDepthRenderPass ( const std::vector<VulkanTexture>& outputs, const RenderPassCreateInfo& ci = { .clearColor_ = false, .clearDepth_ = true, .flags_ = eRenderPassBit_Offscreen | eRenderPassBit_First } );
};


#endif // !__VULKAN_RESOURCES_H__
