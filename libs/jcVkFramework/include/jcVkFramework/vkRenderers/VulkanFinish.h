#ifndef __VULKAN_FINISH_H__
#define __VULKAN_FINISH_H__

#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>

/**
 * @brief VulkanFinish object helps create another empty rendering pass that transitions the swapchain image to the VK_IMAGE_LAYOUT_PRESENT_SRC_KHR format
*/

class VulkanFinish : public RendererBase
{
public:
	VulkanFinish ( VulkanRenderDevice& vkDev, VulkanImage depthTexture );

	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;
};

#endif // __VULKAN_FINISH_H__