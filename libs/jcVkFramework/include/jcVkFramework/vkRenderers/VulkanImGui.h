#ifndef __VULKAN_IM_GUI_H__
#define __VULKAN_IM_GUI_H__

//#include "vkRenderers/VulkanRendererBase.h"
#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>

class ImGuiRenderer : public RendererBase
{
private:
	const ImDrawData* drawData_ = nullptr;

	std::vector<VulkanTexture> extTextures_;
	// storage buffer with index and vertex data
	VkDeviceSize bufferSize_;
	std::vector<VkBuffer> storageBuffer_;
	std::vector<VkDeviceMemory> storageBufferMemory_;

	VkSampler fontSampler_;
	VulkanImage font_;

public:
	explicit ImGuiRenderer ( VulkanRenderDevice& vkDev );
	virtual ~ImGuiRenderer ();

	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;
	void updateBuffers ( VulkanRenderDevice& vkDev, uint32_t currentImage, const ImDrawData* imguiDrawData );

private:
	bool createDescriptorSet ( VulkanRenderDevice& vkDev );

};

#endif // __VULKAN_IM_GUI_H__