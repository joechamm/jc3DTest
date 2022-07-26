#include <jcVkFramework/vkRenderers/VulkanComputedImage.h>

ComputedImage::ComputedImage ( VulkanRenderDevice& vkDev, const char* shaderName, uint32_t textureWidth, uint32_t textureHeight, bool supportDownload )
	: ComputedItem(vkDev, sizeof(uint32_t))
	, computedWidth_(textureWidth)
	, computedHeight_(textureHeight)
	, canDownloadImage_(supportDownload)
{
	createComputedTexture ( textureWidth, textureHeight );
	createComputedImageSetLayout ();

	const VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = sizeof ( float )
	};

	const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &dsLayout_,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange
	};

	VK_CHECK ( vkCreatePipelineLayout ( vkDev_.device, &pipelineLayoutInfo, nullptr, &pipelineLayout_ ) );

	createDescriptorSet ();

	ShaderModule s;
	createShaderModule ( vkDev_.device, &s, shaderName );
	if ( createComputePipeline ( vkDev_.device, s.shaderModule, pipelineLayout_, &pipeline_ ) != VK_SUCCESS )
	{
		printf ( "ComputedImage: failed to create compute pipeline\n" );
		exit ( EXIT_FAILURE );
	}

	vkDestroyShaderModule ( vkDev_.device, s.shaderModule, nullptr );
}

bool ComputedImage::createComputedTexture ( uint32_t computedWidth, uint32_t computedHeight, VkFormat format )
{
	VkFormatProperties fmtProps;
	vkGetPhysicalDeviceFormatProperties ( vkDev_.physicalDevice, format, &fmtProps );

	if ( !(fmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) )
	{
		return false;
	}

	if ( !createImage ( vkDev_.device, vkDev_.physicalDevice,
		computedWidth, computedHeight, format,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | (canDownloadImage_ ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
		!canDownloadImage_ ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		computed_.image, computed_.imageMemory ) )
	{
		return false;
	}

	transitionImageLayout ( vkDev_, computed_.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );
	return createTextureSampler ( vkDev_.device, &computedImageSampler_ ) && createImageView ( vkDev_.device, computed_.image, format, VK_IMAGE_ASPECT_COLOR_BIT, &computed_.imageView );
}

bool ComputedImage::createComputedImageSetLayout ()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1 },
		{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1}
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
		descriptorSetLayoutBinding ( 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT ),
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


bool ComputedImage::createDescriptorSet ()
{
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = 1,
		.pSetLayouts = &dsLayout_
	};

	VK_CHECK ( vkAllocateDescriptorSets ( vkDev_.device, &allocInfo, &descriptorSet_ ));

	const VkDescriptorBufferInfo bufferInfo = {
		.buffer = uniformBuffer_.buffer,
		.offset = 0,
		.range = uniformBuffer_.size
	};

	const VkDescriptorImageInfo imageInfo = {
		.sampler = computedImageSampler_,
		.imageView = computed_.imageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL /*VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL*/
	};

	VkWriteDescriptorSet writeImageDs = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet_,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &imageInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	
	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		VkWriteDescriptorSet {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet_,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &imageInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr
		},
		bufferWriteDescriptorSet ( descriptorSet_, &bufferInfo, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER )
	};

	vkUpdateDescriptorSets ( vkDev_.device, static_cast<uint32_t>(writeDescriptorSets.size ()), writeDescriptorSets.data (), 0, nullptr );

	return true;
}

void ComputedImage::downloadImage ( void* imageData )
{
	if ( !canDownloadImage_ || !imageData )
	{
		return;
	}

	downloadImageData ( vkDev_, computed_.image, computedWidth_, computedHeight_, VK_FORMAT_R8G8B8A8_UNORM, 1, imageData, VK_IMAGE_LAYOUT_GENERAL );
}


