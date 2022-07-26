#include <jcVkFramework/vkRenderers/VulkanComputedItem.h>

ComputedItem::ComputedItem ( VulkanRenderDevice& vkDev, uint32_t uniformBufferSize )
	: vkDev_ ( vkDev )
{
	uniformBuffer_.size = uniformBufferSize;

	VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	if ( vkCreateFence ( vkDev.device, &fenceCreateInfo, nullptr, &fence_ ) != VK_SUCCESS )
	{
		printf ( "ComputedItem: failed to create fence\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createUniformBuffer ( vkDev, uniformBuffer_.buffer, uniformBuffer_.memory, uniformBuffer_.size ) )
	{
		printf ( "ComputedItem: failed to create uniform buffer\n" );
		exit ( EXIT_FAILURE );
	}
}

ComputedItem::~ComputedItem ()
{
	vkDestroyBuffer ( vkDev_.device, uniformBuffer_.buffer, nullptr );
	vkFreeMemory ( vkDev_.device, uniformBuffer_.memory, nullptr );

	vkDestroyFence ( vkDev_.device, fence_, nullptr );

	vkDestroyDescriptorPool ( vkDev_.device, descriptorPool_, nullptr );
	vkDestroyDescriptorSetLayout ( vkDev_.device, dsLayout_, nullptr );
	vkDestroyPipeline ( vkDev_.device, pipeline_, nullptr );
	vkDestroyPipelineLayout ( vkDev_.device, pipelineLayout_, nullptr );
}

void ComputedItem::fillComputeComandBuffer ( void* pushConstant, uint32_t pushConstantSize, uint32_t xsize, uint32_t ysize, uint32_t zsize )
{
	VkCommandBuffer commandBuffer = vkDev_.computeCommandBuffer;

	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK ( vkBeginCommandBuffer ( commandBuffer, &beginInfo ) );

	vkCmdBindPipeline ( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_ );
	vkCmdBindDescriptorSets ( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout_, 0, 1, &descriptorSet_, 0, 0 );

	if ( pushConstant && pushConstantSize > 0 )
	{
		vkCmdPushConstants ( commandBuffer, pipelineLayout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, pushConstantSize, pushConstant );
	}

	vkCmdDispatch ( commandBuffer, xsize, ysize, zsize );
	vkEndCommandBuffer ( commandBuffer );
}

bool ComputedItem::submit ()
{
	// Use a fence to ensure that compute command buffer has finished executing before using it again
	waitFence ();

	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &vkDev_.computeCommandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};

	return (vkQueueSubmit ( vkDev_.computeQueue, 1, &submitInfo, fence_ ) == VK_SUCCESS);
}

void ComputedItem::waitFence ()
{
	vkWaitForFences ( vkDev_.device, 1, &fence_, VK_TRUE, UINT64_MAX );
	vkResetFences ( vkDev_.device, 1, &fence_ );
}