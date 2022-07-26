#ifndef __VULKAN_CUBE_H__
#define __VULKAN_CUBE_H__

//#include "vkRenderers/VulkanRendererBase.h"
#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class CubeRenderer : public RendererBase
{
private:
	
	VkSampler textureSampler_;
	VulkanImage texture_;

public:
	CubeRenderer ( VulkanRenderDevice& vkDev, VulkanImage inDepthTexture, const char* textureFile );
	virtual ~CubeRenderer ();

	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;
	void updateUniformBuffer ( VulkanRenderDevice& vkDev, uint32_t currentImage, const glm::mat4& m );

private:
	bool createDescriptorSet ( VulkanRenderDevice& vkDev );

};

#endif // __VULKAN_CUBE_H__