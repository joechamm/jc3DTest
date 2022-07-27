#include <jcVkFramework/vkRenderers/VulkanComputedVB.h>

ComputedVertexBuffer::ComputedVertexBuffer ( VulkanRenderDevice& vkDev, const char* shaderName,
	uint32_t indexBufferSize,
	uint32_t uniformBufferSize,
	uint32_t vertexSize,
	uint32_t vertexCount,
	bool supportDownload )
	: ComputedItem ( vkDev, uniformBufferSize )
	, computedVertexCount_ ( vertexCount )
	, indexBufferSize_ ( indexBufferSize )
	, vertexSize_ ( vertexSize )
	, canDownloadVertices_ ( supportDownload )
{
	createComputedBuffer ();
	createComputedSetLayout ();
	createPipelineLayout ( vkDev.device, dsLayout_, &pipelineLayout_ );

	createDescriptorSet ();

	ShaderModule s;
	createShaderModule ( vkDev.device, &s, shaderName );
	if ( createComputePipeline ( vkDev.device, s.shaderModule, pipelineLayout_, &pipeline_ ) != VK_SUCCESS )
	{
		printf ( "ComputedVertexBuffer: failed to create compute pipeline\n" );
		exit ( EXIT_FAILURE );
	}

	vkDestroyShaderModule ( vkDev.device, s.shaderModule, nullptr );
}

bool ComputedVertexBuffer::createComputedBuffer ()
{
	return createBuffer ( vkDev_.device, vkDev_.physicalDevice,
		computedVertexCount_ * vertexSize_ + indexBufferSize_,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | (canDownloadVertices_ ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0) | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		!canDownloadVertices_ ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		computedBuffer_, computedMemory_ );
}

bool ComputedVertexBuffer::createComputedSetLayout ()
{
	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1 },
		{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1}
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = 1,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size ()),
		.pPoolSizes = poolSizes.data ()
	};

	VK_CHECK ( vkCreateDescriptorPool ( vkDev_.device, &descriptorPoolInfo, nullptr, &descriptorPool_ ) );

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
		descriptorSetLayoutBinding ( 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT ),
		descriptorSetLayoutBinding ( 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT )
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size ()),
		.pBindings = bindings.data ()
	};

	VK_CHECK ( vkCreateDescriptorSetLayout ( vkDev_.device, &layoutInfo, nullptr, &dsLayout_ ) );

	return true;
}

bool ComputedVertexBuffer::createDescriptorSet ()
{
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = 1,
		.pSetLayouts = &dsLayout_
	};

	VK_CHECK ( vkAllocateDescriptorSets ( vkDev_.device, &allocInfo, &descriptorSet_ ) );

	const VkDescriptorBufferInfo bufferInfo = { computedBuffer_, 0, computedVertexCount_ * vertexSize_ };
	const VkDescriptorBufferInfo bufferInfo2 = { uniformBuffer_.buffer, 0, uniformBuffer_.size };

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		bufferWriteDescriptorSet ( descriptorSet_, &bufferInfo, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ),
		bufferWriteDescriptorSet ( descriptorSet_, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER )
	};

	vkUpdateDescriptorSets ( vkDev_.device, static_cast<uint32_t>(writeDescriptorSets.size ()), writeDescriptorSets.data (), 0, nullptr );

	return true;
}

void ComputedVertexBuffer::downloadVertices ( void* vertexData )
{
	if ( !canDownloadVertices_ || !vertexData )
	{
		return;
	}

	downloadBufferData ( vkDev_, computedMemory_, 0, vertexData, computedVertexCount_ * vertexSize_ );
}

void ComputedVertexBuffer::uploadIndexData ( uint32_t* indices )
{
	uploadBufferData ( vkDev_, computedMemory_, computedVertexCount_ * vertexSize_, indices, indexBufferSize_ );
}