#include <jcVkFramework/vkRenderers/VulkanComputeBase.h>

ComputeBase::ComputeBase ( VulkanRenderDevice& vkDev, const char* shaderName, uint32_t inputSize, uint32_t outputSize )
	: vkDev_ ( vkDev )
{
	/* Both inBuffer_ and outBuffer_ are allocated as 'host-visible'. If the output buffer is needed only for rendering purposes, host visibility and coherence can be disabled. */
	createSharedBuffer ( vkDev, inputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		inBuffer_, inBufferMemory_ );

	createSharedBuffer ( vkDev, outputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		outBuffer_, outBufferMemory_ );

	ShaderModule s;
	createShaderModule ( vkDev.device, &s, shaderName );

	createComputeDescriptorSetLayout ( vkDev.device, &dsLayout_ );
	createPipelineLayout ( vkDev.device, dsLayout_, &pipelineLayout_ );
	createComputePipeline ( vkDev.device, s.shaderModule, pipelineLayout_, &pipeline_ );
	createComputeDescriptorSet ( vkDev.device, dsLayout_ );

	vkDestroyShaderModule ( vkDev.device, s.shaderModule, nullptr );	
}

ComputeBase::~ComputeBase ()
{
	vkDestroyBuffer ( vkDev_.device, inBuffer_, nullptr );
	vkFreeMemory ( vkDev_.device, inBufferMemory_, nullptr );

	vkDestroyBuffer ( vkDev_.device, outBuffer_, nullptr );
	vkFreeMemory ( vkDev_.device, outBufferMemory_, nullptr );

	vkDestroyPipelineLayout ( vkDev_.device, pipelineLayout_, nullptr );
	vkDestroyPipeline ( vkDev_.device, pipeline_, nullptr );

	vkDestroyDescriptorSetLayout ( vkDev_.device, dsLayout_, nullptr );
	vkDestroyDescriptorPool ( vkDev_.device, descriptorPool_, nullptr );
}

bool ComputeBase::createComputeDescriptorSet ( VkDevice device, VkDescriptorSetLayout descriptorSetLayout )
{
	// Descriptor Pool
	VkDescriptorPoolSize descriptorPoolSize = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 };

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 0, 0, 1, 1, &descriptorPoolSize
	};

	VK_CHECK ( vkCreateDescriptorPool ( device, &descriptorPoolCreateInfo, 0, &descriptorPool_ ) );

	// Descriptor Set
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		0, descriptorPool_, 1, &descriptorSetLayout
	};

	VK_CHECK ( vkAllocateDescriptorSets ( device, &descriptorSetAllocInfo, &descriptorSet_ ) );

	// finally, update descriptor set with concrete buffer pointers
	VkDescriptorBufferInfo inBufferInfo = { inBuffer_, 0, VK_WHOLE_SIZE };
	VkDescriptorBufferInfo outBufferInfo = { outBuffer_, 0, VK_WHOLE_SIZE };

	VkWriteDescriptorSet writeDescriptorSet[2] = {
		{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 0, descriptorSet_, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &inBufferInfo, 0 },
		{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 0, descriptorSet_, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &outBufferInfo, 0 }
	};

	vkUpdateDescriptorSets ( device, 2, writeDescriptorSet, 0, 0 );
	
	return true;
}