#ifndef __VULKAN_MODEL_RENDERER_H__
#define __VULKAN_MODEL_RENDERER_H__

//#include "vkRenderers/VulkanRendererBase.h"
#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>

/**
 * @brief The ModelRenderer class contains a texture and combined vertex and index buffers
*/

class ModelRenderer : public RendererBase
{
private:
	size_t vertexBufferSize_;
	size_t indexBufferSize_;
	VkBuffer storageBuffer_;
	VkDeviceMemory storageBufferMemory_;

	VkSampler textureSampler_;
	VulkanImage texture_;

public:
	ModelRenderer ( VulkanRenderDevice& vkDev, const char* modelFile, const char* textureFile, uint32_t uniformDataSize );
	virtual ~ModelRenderer ();
	
	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;

	void updateUniformBuffer ( VulkanRenderDevice& vkDev, uint32_t currentImage, const void* data, size_t dataSize );

private:
	bool createDescriptorSet ( VulkanRenderDevice& vkDev, uint32_t uniformDataSize );
};


#endif // __VULKAN_MODEL_RENDERER_H__