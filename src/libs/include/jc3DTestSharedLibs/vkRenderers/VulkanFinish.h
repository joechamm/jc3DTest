#pragma once

#include <jc3DTestSharedLibs/vkRenderers/VulkanRendererBase.h>

class VulkanFinish: public RendererBase
{
public:
	VulkanFinish(VulkanRenderDevice& vkDev, VulkanImage depthTexture);

	virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;
};
