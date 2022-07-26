#include <jcVkFramework/vkRenderers/VulkanSingleQuad.h>
#include <jcVkFramework/EasyProfilerWrapper.h>
#include <jcVkFramework/ResourceString.h>

bool VulkanSingleQuadRenderer::createDescriptorSet ( VulkanRenderDevice& vkDev, VkImageLayout desiredLayout )
{
	VkDescriptorSetLayoutBinding binding = descriptorSetLayoutBinding ( 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT );

	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = 1,
		.pBindings = &binding
	};

	VK_CHECK ( vkCreateDescriptorSetLayout ( vkDev.device, &layoutInfo, nullptr, &descriptorSetLayout_ ) );

	std::vector<VkDescriptorSetLayout> layouts ( vkDev.swapchainImages.size (), descriptorSetLayout_ );

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(vkDev.swapchainImages.size ()),
		.pSetLayouts = layouts.data ()
	};

	descriptorSets_.resize ( vkDev.swapchainImages.size () );
	VK_CHECK ( vkAllocateDescriptorSets ( vkDev.device, &allocInfo, descriptorSets_.data () ) );

	VkDescriptorImageInfo textureDescriptor = VkDescriptorImageInfo{
		.sampler = textureSampler_,
		.imageView = texture_.imageView,
		.imageLayout = desiredLayout
	};

	// for each swapchainImage, update the corresponding descriptorSet with specific buffer handles
	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		VkWriteDescriptorSet imageDescriptorWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets_[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &textureDescriptor
		};

		vkUpdateDescriptorSets ( vkDev.device, 1, &imageDescriptorWrite, 0, nullptr );
	}

	return true;
}

void VulkanSingleQuadRenderer::fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage )
{
	beginRenderPass ( commandBuffer, currentImage );

	vkCmdDraw ( commandBuffer, 6, 1, 0, 0 );
	vkCmdEndRenderPass ( commandBuffer );
}

VulkanSingleQuadRenderer::VulkanSingleQuadRenderer ( VulkanRenderDevice& vkDev, VulkanImage tex, VkSampler sampler, VkImageLayout desiredLayout)
	: vkDev_(vkDev)
	, texture_(tex)
	, textureSampler_(sampler)
	, RendererBase ( vkDev, VulkanImage () )

{
	if(!createUniformBuffers(vkDev, sizeof(uint32_t)) ||
		!createDescriptorPool(vkDev, 0, 0, 1, &descriptorPool_) ||
		!createDescriptorSet(vkDev, desiredLayout) ||
		!createColorAndDepthRenderPass(vkDev, false, &renderPass_, RenderPassCreateInfo()) ||
		!createPipelineLayout ( vkDev.device, descriptorSetLayout_, &pipelineLayout_ ) ||
		!createGraphicsPipeline(vkDev, renderPass_, pipelineLayout_, getShaderFilenamesWithRoot("assets/shaders/VK06_quad.vert","assets/shaders/VK06_quad.frag"), &graphicsPipeline_) )
	{
		printf ( "VulkanSingleQuadRenderer: Failed to create pipeline\n" );
		fflush ( stdout );
		exit ( 0 );
	}

	createColorAndDepthFramebuffers ( vkDev, renderPass_, VK_NULL_HANDLE, swapchainFramebuffers_ );
}

VulkanSingleQuadRenderer::~VulkanSingleQuadRenderer ()
{
}