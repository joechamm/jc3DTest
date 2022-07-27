#ifndef __VULKAN_MODEL_RENDERER_H__
#define __VULKAN_MODEL_RENDERER_H__

#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>

/**
 * @brief The ModelRenderer class contains a texture and combined vertex and index buffers
*/

class ModelRenderer : public RendererBase
{
private:
	bool useGeneralTextureLayout_ = false;
	bool isExternalDepth_ = false;
	bool deleteMeshData_ = true;

	size_t vertexBufferSize_;
	size_t indexBufferSize_;

	// 6. Storage buffer with index and vertex data
	VkBuffer storageBuffer_;
	VkDeviceMemory storageBufferMemory_;

	VkSampler textureSampler_;
	VulkanImage texture_;

public:
	ModelRenderer ( VulkanRenderDevice& vkDev, const char* modelFile, const char* textureFile, uint32_t uniformDataSize );
	ModelRenderer ( VulkanRenderDevice& vkDev, bool useDepth, VkBuffer storageBuffer, VkDeviceMemory storageBufferMemory, uint32_t vertexBufferSize, uint32_t indexBufferSize, VulkanImage texture, VkSampler textureSampler, const std::vector<std::string>& shaderFiles, uint32_t uniformDataSize, bool useGeneralTextureLayout = true, VulkanImage externalDepth = { .image = VK_NULL_HANDLE }, bool deleteMeshData = true );
	virtual ~ModelRenderer ();
	
	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;

	void updateUniformBuffer ( VulkanRenderDevice& vkDev, uint32_t currentImage, const void* data, size_t dataSize );

	// HACK to allow sharing textures between multiple ModelRenderers
	void freeTextureSampler ()
	{
		textureSampler_ = VK_NULL_HANDLE; 
	}

private:
	bool createDescriptorSet ( VulkanRenderDevice& vkDev, uint32_t uniformDataSize );
};


#endif // __VULKAN_MODEL_RENDERER_H__