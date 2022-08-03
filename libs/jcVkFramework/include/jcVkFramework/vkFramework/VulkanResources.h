#ifndef __VULKAN_RESOURCES_H__
#define __VULKAN_RESOURCES_H__

#include <jcVkFramework/UtilsVulkan.h>
#include <volk/volk.h>

#include <cstring>
#include <memory>
#include <map>
#include <utility>

/**
 * For more or less abstract descriptor set setup we need to describe individual items ("bindings").
 * These are buffers, textures (samplers, but we call them "textures" here) and arrays of textures.
 */

/// Common information for all bindings (buffer/sampler type and shader stage(s) where this item is used)
struct DescriptorInfo
{
	VkDescriptorType type_;
	VkShaderStageFlags shaderStageFlags_;
};

struct BufferAttachment
{
	DescriptorInfo dInfo_;

	VulkanBuffer buffer_;
	uint32_t offset_;
	uint32_t size_;
};

struct TextureAttachment
{
	DescriptorInfo dInfo_;
	VulkanTexture texture_;
};

struct TextureArrayAttachment
{
	DescriptorInfo dInfo_;
	std::vector<VulkanTexture> textureArray_;
};

struct TextureCreationParams
{
	VkFilter minFilter_;
	VkFilter maxFilter_;

/// VkAccessModeFlags accessMode_;
};

inline TextureAttachment makeTextureAttachment ( VulkanTexture tex, VkShaderStageFlags shaderStageFlags )
{
	return TextureAttachment {
		.dInfo_ = {
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderStageFlags_ = shaderStageFlags,
		},		
		.texture_ = tex
	};
}

inline TextureAttachment fsTextureAttachment ( VulkanTexture tex )
{
	return makeTextureAttachment ( tex, VK_SHADER_STAGE_FRAGMENT_BIT );
}

inline TextureArrayAttachment fsTextureArrayAttachment ( const std::vector<VulkanTexture>& textures )
{
	return TextureArrayAttachment {
		.dInfo_ = {
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderStageFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
		},		
		.textureArray_ = textures
	};
}

inline BufferAttachment makeBufferAttachment ( VulkanBuffer buffer, uint32_t offset, uint32_t size, VkDescriptorType type, VkShaderStageFlags shaderStageFlags )
{
	return BufferAttachment {
		.dInfo_ = {
			.type_ = type,
			.shaderStageFlags_ = shaderStageFlags,
		},		
		.buffer_ = {
			.buffer = buffer.buffer,
			.size = buffer.size,
			.memory = buffer.memory,
			.ptr = buffer.ptr
		},
		.offset_ = offset,
		.size_ = size
	};
}

inline BufferAttachment uniformBufferAttachment ( VulkanBuffer buffer, uint32_t offset, uint32_t size, VkShaderStageFlags shaderStageFlags )
{
	return makeBufferAttachment ( buffer, offset, size, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, shaderStageFlags );
}

inline BufferAttachment storageBufferAttachment(VulkanBuffer buffer, uint32_t offset, uint32_t size, VkShaderStageFlags shaderStageFlags)
{
	return makeBufferAttachment(buffer, offset, size, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, shaderStageFlags);
}

/* Aggregate structure with all the data for descriptor set (or descriptor set layout) allocation) */
struct DescriptorSetInfo
{
	std::vector<BufferAttachment> buffers_;
	std::vector<TextureAttachment> textures_;
	std::vector<TextureArrayAttachment> textureArrays_;
};

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

/**
	The VulkanResources class keeps track of all allocated/created Vulkan-related objects
	The list of textures, buffers and descriptor pools.
	Individual methods from the load*() serie implement all loading
	The postprocessing chain is a "mixture" of a rendering technique, fullscreen postprocessing pipeline, resource manager and a frame graph
	Since RenderingTechnique, FrameGraph and PostprocessingPipeline all directly fill the list of rendering API commands,
	it is natural to generalize all rendering operations here.
	The "resources" part of this class is a set of methods to ease allocation and loading of textures and buffers.
	All allocated/loaded textures and buffers are automatically destroyed in the end.
	On top of that class we may add a (pretty simple even without reflection) serialization routine
	which reads a list of used buffers/textures, all the processing/rendering steps, internal parameter names
	and inter-stage dependencies.
*/
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

	inline VulkanBuffer addUniformBuffer ( VkDeviceSize bufferSize, bool createMapping = false )
	{
		return addBuffer ( bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, createMapping );
	}

	inline VulkanBuffer addStorageBuffer ( VkDeviceSize bufferSize, bool createMapping = false )
	{
		return addBuffer ( bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | /* for debugging make it host-visible */ VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, createMapping);
	}

	inline VulkanBuffer addIndirectBuffer ( VkDeviceSize bufferSize, bool createMapping = false )
	{
		return addBuffer ( bufferSize, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, // VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | /* for debugging make it host-visible */ VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, createMapping );
	}

	inline VulkanBuffer addComputedIndirectBuffer ( VkDeviceSize bufferSize, bool createMapping = false )
	{
		return addBuffer ( bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | /* for debugging make it host-visible */ VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, createMapping );
	}

	/* Allocate and upload vertex & index buffer pair */
	VulkanBuffer addVertexBuffer (uint32_t indexBufferSize, const void* indexData, uint32_t vertexBufferSize, const void* vertexData  );

	VkFramebuffer addFramebuffer ( RenderPass renderPass, const std::vector<VulkanTexture>& images );

	RenderPass addFullScreenPass ( bool useDepth = true, const RenderPassCreateInfo& ci = RenderPassCreateInfo () );

	RenderPass addRenderPass ( const std::vector<VulkanTexture>& outputs, const RenderPassCreateInfo& ci = { .clearColor_ = false, .clearDepth_ = true, .flags_ = eRenderPassBit_Offscreen | eRenderPassBit_First }, bool useDepth = true );
	RenderPass addDepthRenderPass ( const std::vector<VulkanTexture>& outputs, const RenderPassCreateInfo& ci = { .clearColor_ = false, .clearDepth_ = true, .flags_ = eRenderPassBit_Offscreen | eRenderPassBit_First } );

	VkPipelineLayout addPipelineLayout (VkDescriptorSetLayout dsLayout, uint32_t vtxConstSize = 0, uint32_t fragConstSize = 0 );
	VkPipeline addPipeline (VkRenderPass renderPass, VkPipelineLayout layout, const std::vector<std::string>& shaderNames, const PipelineInfo& pi = PipelineInfo {.width_ = 0, .height_ = 0, .topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, .useDepth_ = true, .useBlending_ = false, .dynamicScissorState_ = false, .patchControlPoints_ = 0 });

	/* Calculate the descriptor pool size from the list of buffers and textures */
	VkDescriptorPool addDescriptorPool ( const DescriptorSetInfo& dsInfo, uint32_t dSetCount = 1 );
	VkDescriptorSetLayout addDescriptorSetLayout ( const DescriptorSetInfo& dsInfo );	
	VkDescriptorSet addDescriptorSet ( VkDescriptorPool descriptorPool, VkDescriptorSetLayout dsLayout );
	void updateDescriptorSet ( VkDescriptorSet ds, const DescriptorSetInfo& dsInfo );

	std::vector<VkFramebuffer> addFramebuffers ( VkRenderPass renderPass, VkImageView depthView = VK_NULL_HANDLE );
};


#endif // !__VULKAN_RESOURCES_H__
