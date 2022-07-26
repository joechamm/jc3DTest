#ifndef __VULKAN_CLEAR_H__
#define __VULKAN_CLEAR_H__

//#include "vkRenderers/VulkanRendererBase.h"
#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>

/* The VulkanClear object initializes and starts an empty rendering pass whose only purpose is to clear the color and depth buffers */

class VulkanClear : public RendererBase
{
private:
	bool shouldClearDepth;

public:
	VulkanClear ( VulkanRenderDevice& vkDev, VulkanImage depthTexture );

	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;
};


#endif // __VULKAN_CLEAR_H__

