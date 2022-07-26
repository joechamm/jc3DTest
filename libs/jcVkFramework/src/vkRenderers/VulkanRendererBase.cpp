//#include "vkRenderers/VulkanRendererBase.h"
#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>
#include <stdio.h>

RendererBase::~RendererBase ()
{
	for ( auto buf : uniformBuffers_ )
	{
		vkDestroyBuffer ( device_, buf, nullptr );
	}

	for ( auto mem : uniformBuffersMemory_ )
	{
		vkFreeMemory ( device_, mem, nullptr );
	}

	vkDestroyDescriptorSetLayout ( device_, descriptorSetLayout_, nullptr );
	vkDestroyDescriptorPool ( device_, descriptorPool_, nullptr );

	for ( auto framebuffer : swapchainFramebuffers_ )
	{
		vkDestroyFramebuffer ( device_, framebuffer, nullptr );
	}

	vkDestroyRenderPass ( device_, renderPass_, nullptr );
	vkDestroyPipelineLayout ( device_, pipelineLayout_, nullptr );
	vkDestroyPipeline ( device_, graphicsPipeline_, nullptr );
}

// beginRenderPass() routine marks the start of a rendering pass and does graphics pipeline binding and
// descritpor set binding for the current image in the swapchain
void RendererBase::beginRenderPass ( VkCommandBuffer commandBuffer, size_t currentImage )
{
	const VkRect2D screenRect = {
		.offset = { 0, 0 },
		.extent = {.width = framebufferWidth_, .height = framebufferHeight_ }
	};

	const VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = renderPass_,
		.framebuffer = swapchainFramebuffers_[currentImage],
		.renderArea = screenRect
	};

	vkCmdBeginRenderPass ( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

	vkCmdBindPipeline ( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_ );

	vkCmdBindDescriptorSets (
		/* VkCommandBuffer commandBuffer */					commandBuffer
		,/* VkPipelineBindPoint pipelineBindPoint */		VK_PIPELINE_BIND_POINT_GRAPHICS
		,/* VkPipelineLayout layout */						pipelineLayout_
		,/* uint32_t firstSet */							0
		,/* uint32_t descriptorSetCount */					1
		,/* const VkDescriptorSet *pDescriptorSets */		&descriptorSets_[currentImage]
		, /* uint32_t dynamicOffsetCount */					0
		,/* const uint32_t *pDynamicOffsets */				nullptr
	);
}

// simple loop over all swapchain images that calls createUniformBuffer() for each
bool RendererBase::createUniformBuffers ( VulkanRenderDevice& vkDev, size_t uniformDataSize )
{
	uniformBuffers_.resize ( vkDev.swapchainImages.size () );
	uniformBuffersMemory_.resize ( vkDev.swapchainImages.size () );
	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		if ( !createUniformBuffer ( vkDev, uniformBuffers_[i], uniformBuffersMemory_[i], uniformDataSize ) )
		{
			printf ( "Cannot create uniform buffer\n" );
			return false;
		}
	}
	return true;
}