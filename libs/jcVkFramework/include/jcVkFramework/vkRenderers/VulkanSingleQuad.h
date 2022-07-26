#ifndef __VULKAN_SINGLE_QUAD_H__
#define __VULKAN_SINGLE_QUAD_H__

#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>

class VulkanSingleQuadRenderer : public RendererBase
{
	VulkanRenderDevice& vkDev_;
	VulkanImage texture_;
	VkSampler textureSampler_;
public:
	VulkanSingleQuadRenderer ( VulkanRenderDevice& vkDev, VulkanImage tex, VkSampler sampler, VkImageLayout desiredLayout = VK_IMAGE_LAYOUT_GENERAL );
	virtual ~VulkanSingleQuadRenderer ();

	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;
private:

	bool createDescriptorSet ( VulkanRenderDevice& vkDev, VkImageLayout desiredLayout = VK_IMAGE_LAYOUT_GENERAL );
};

#endif // !__VULKAN_SINGLE_QUAD_H__

