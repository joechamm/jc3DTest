#ifndef __VULKAN_COMPUTED_IMAGE_H__
#define __VULKAN_COMPUTED_IMAGE_H__

#include <jcCommonFramework/Utils.h>
#include <jcVkFramework/vkRenderers/VulkanComputedItem.h>

struct ComputedImage : public ComputedItem
{
	VulkanImage computed_;
	VkSampler computedImageSampler_;
	uint32_t computedWidth_;
	uint32_t computedHeight_;
protected:
	bool canDownloadImage_;

public:
	ComputedImage ( VulkanRenderDevice& vkDev, const char* shaderName, uint32_t textureWidth, uint32_t textureHeight, bool supportDownload = false );
	virtual ~ComputedImage ()
	{}

	void downloadImage ( void* imageData );

protected:
	bool createComputedTexture ( uint32_t computedWidth, uint32_t computedHeight, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM );

	bool createDescriptorSet ();
	bool createComputedImageSetLayout ();
};

#endif // !__VULKAN_COMPUTED_IMAGE_H__

