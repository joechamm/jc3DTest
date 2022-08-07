#ifndef __BARRIERS_H__
#define __BARRIERS_H__

#include <jcVkFramework/vkFramework/Renderer.h>

struct ShaderOptimalToColorBarrier : public Renderer
{
private:
	VulkanTexture tex_;

public:
	ShaderOptimalToColorBarrier ( VulkanRenderContext& c, VulkanTexture tex )
		: Renderer ( c )
		, tex_ ( tex )
	{}

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE ) override
	{
		transitionImageLayoutCmd ( cmdBuffer, tex_.image.image, tex_.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	}
};

struct ShaderOptimalDepthBarrier : public Renderer
{
private:
	VulkanTexture tex_;

public:
	ShaderOptimalToDepthBarrier ( VulkanRenderContext& c, VulkanTexture tex )
		: Renderer ( c )
		, tex_ ( tex )
	{}

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VulkanRenderPass rp = VK_NULL_HANDLE ) override
	{
		transitionImageLayoutCmd ( cmdBuffer, tex_.image.image, tex_.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	}
};

struct ColorToShaderOptimalBarrier : public Renderer
{
private:
	VulkanTexture tex_;

public:
	ColorToShaderOptimalBarrier(VulkanRenderContext& c, VulkanTexture tex)
		: Renderer(c)
		, tex_(tex) {}

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VulkanRenderPass rp = VK_NULL_HANDLE ) override
	{
		transitionImageLayoutCmd ( cmdBuffer, tex_.image.image, tex_.format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	}
};

struct ColorWaitBarrier : public Renderer
{
private:
	VulkanTexture tex_;

public:
	ColorWaitBarrier ( VulkanRenderContext& c, VulkanTexture tex )
		: Renderer ( c )
		, tex_ ( tex )
	{}

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VulkanRenderPass rp = VK_NULL_HANDLE ) override
	{
		transitionImageLayoutCmd ( cmdBuffer, tex_.image.image, tex_.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	}
};

struct DepthToShaderOptimalBarrier : public Renderer
{
private:
	VulkanTexture tex_;

public:
	DepthToShaderOptimalBarrier ( VulkanRenderContext& c, VulkanTexture tex )
		: Renderer ( c )
		, tex_ ( tex )
	{}

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VulkanRenderPass rp = VK_NULL_HANDLE ) override
	{
		transitionImageLayoutCmd ( cmdBuffer, tex_.image.image, tex_.format, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	}
};

struct ImageBarrier : public Renderer
{
private:
	VkAccessFlags srcAccess_;
	VkAccessFlags dstAccess_;
	VkImageLayout oldLayout_;
	VkImageLayout newLayout_;
	VkImage image_;

public:
	ImageBarrier(VulkanRenderContext& c, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image) 
		: Renderer(c)
		, srcAccess_(srcAccess)
		, dstAccess_(dstAccess)
		, oldLayout_(oldLayout)
		, newLayout_(newLayout)
		, image_(image)
	{}

	virtual void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE )
	{
		VkImageMemoryBarrier barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = srcAccess_,
			.dstAccessMask = dstAccess_,
			.oldLayout = oldLayout_,
			.newLayout = newLayout_,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image_,
			.subresourceRange = VkImageSubresourceRange {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		vkCmdPipelineBarrier (
			cmdBuffer,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}
};

#endif // !__BARRIERS_H__
