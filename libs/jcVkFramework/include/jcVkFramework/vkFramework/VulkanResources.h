#ifndef __VULKAN_RESOURCES_H__
#define __VULKAN_RESOURCES_H__

#include <jcVkFramework/UtilsVulkan.h>
#include <volk/volk.h>

#include <cstring>
#include <memory>
#include <map>
#include <utility>

/* A structure with pipeline parameters */
struct PipelineInfo
{
	uint32_t width_ = 0;
	uint32_t height_ = 0;
	
	VkPrimitiveTopology topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	
	bool useDepth_ = true;	
	bool useBlending_ = true;	
	bool dynamicScissorState_ = false;

	uint32_t patchControlPoints_ = 0;
};

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

	/* Allocate and upload vertex & index buffer pair */
	VulkanBuffer addVertexBuffer (uint32_t indexBufferSize, const void* indexData, uint32_t vertexBufferSize, const void* vertexData  );

	VkFramebuffer addFramebuffer ( RenderPass renderPass, const std::vector<VulkanTexture>& images );

	RenderPass addFullScreenPass ( bool useDepth = true, const RenderPassCreateInfo& ci = RenderPassCreateInfo () );

	RenderPass addRenderPass ( const std::vector<VulkanTexture>& outputs, const RenderPassCreateInfo& ci = { .clearColor_ = false, .clearDepth_ = true, .flags_ = eRenderPassBit_Offscreen | eRenderPassBit_First }, bool useDepth = true );
	RenderPass addDepthRenderPass ( const std::vector<VulkanTexture>& outputs, const RenderPassCreateInfo& ci = { .clearColor_ = false, .clearDepth_ = true, .flags_ = eRenderPassBit_Offscreen | eRenderPassBit_First } );

	VkPipelineLayout addPipelineLayout (VkDescriptorSetLayout dsLayout, uint32_t vtxConstSize = 0, uint32_t fragConstSize = 0 );
	VkPipeline addPipeline (VkRenderPass renderPass, VkPipelineLayout layout, const std::vector<std::string>& shaderNames, const PipelineInfo& pi = PipelineInfo {.width_ = 0, .height_ = 0, .topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, .useDepth_ = true, .useBlending_ = false, .dynamicScissorState_ = false, .patchControlPoints_ = 0 });

	std::vector<VkFramebuffer> addFramebuffers ( VkRenderPass renderPass, VkImageView depthView = VK_NULL_HANDLE );
};


#endif // !__VULKAN_RESOURCES_H__
