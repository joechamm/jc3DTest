#include "vkRenderers/VulkanClear.h"
#include "EasyProfilerWrapper.h"

VulkanClear::VulkanClear ( VulkanRenderDevice& vkDev, VulkanImage depthTexture )
	: RendererBase ( vkDev, depthTexture )
	, shouldClearDepth ( depthTexture.image != VK_NULL_HANDLE )
{
	// The RenderPassCreateInfo structure defines how this rendering pass should be created. The eRenderPassBit_First
	// flag defines a rendering pass as the first pass, which means that before this rendering pass, our swapchain image
	// should be inn the VK_LAYOUT_UNDEFINED state and that after this pass it is not yet suitable for presentation but
	// only for rendering
	if ( !createColorAndDepthRenderPass ( vkDev, shouldClearDepth, &renderPass_, RenderPassCreateInfo{
		.clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First } ) )
	{
		printf ( "VulkanClear: failed to create render pass\n" );
		exit ( EXIT_FAILURE );
	}

	createColorAndDepthFramebuffers ( vkDev, renderPass_, depthTexture.imageView, swapchainFramebuffers_ );
}

void VulkanClear::fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t swapFramebuffer )
{
	EASY_FUNCTION ();

	const VkClearValue clearValues[2] = {
		VkClearValue{ .color = { 1.0f, 1.0f, 1.0f, 1.0f } },
		VkClearValue { .depthStencil = { 1.0f, 0 } }
	};

	const VkRect2D screenRect = {
		.offset = { 0, 0 },
		.extent = {.width = framebufferWidth_, .height = framebufferHeight_ }
	};

	const VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass_,
		.framebuffer = swapchainFramebuffers_[swapFramebuffer],
		.renderArea = screenRect,
		.clearValueCount = static_cast<uint32_t>(shouldClearDepth ? 2u : 1u),
		.pClearValues = &clearValues[0]
	};

	vkCmdBeginRenderPass ( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
	vkCmdEndRenderPass ( commandBuffer );
}