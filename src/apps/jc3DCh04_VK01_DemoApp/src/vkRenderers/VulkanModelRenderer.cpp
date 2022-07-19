#include "vkRenderers/VulkanModelRenderer.h"

#include "ResourceString.h"


ModelRenderer::ModelRenderer ( VulkanRenderDevice& vkDev, const char* modelFile, const char* textureFile, uint32_t uniformDataSize ) 
	: RendererBase ( vkDev, VulkanImage () )
{
	if ( !createTexturedVertexBuffer ( vkDev, modelFile, &storageBuffer_, &storageBufferMemory_, &vertexBufferSize_, &indexBufferSize_ ) )
	{
		printf ( "ModelRenderer: createTexturedVertexBuffer failed\n" );
		exit ( EXIT_FAILURE );
	}

	createTextureImage ( vkDev, textureFile, texture_.image, texture_.imageMemory );
	createImageView ( vkDev.device, texture_.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &texture_.imageView );
	createTextureSampler ( vkDev.device, &textureSampler_ );

	if ( !createDepthResources ( vkDev, vkDev.framebufferWidth, vkDev.framebufferHeight, depthTexture_ ) )
	{
		printf ( "ModelRenderer: failed to create depth resources\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createColorAndDepthRenderPass ( vkDev, true, &renderPass_, RenderPassCreateInfo() ) )
	{
		printf ( "ModelRenderer: failed to create color and depth render pass\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createUniformBuffers ( vkDev, uniformDataSize ) )
	{
		printf ( "ModelRenderer: failed to create uniform buffers\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createColorAndDepthFramebuffers ( vkDev, renderPass_, depthTexture_.imageView, swapchainFramebuffers_) )
	{
		printf ( "ModelRenderer: failed to create color and depth framebuffers\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createDescriptorPool ( vkDev, 1, 2, 1, &descriptorPool_ ) )
	{
		printf ( "ModelRenderer: failed to create descriptor pool\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createDescriptorSet ( vkDev, uniformDataSize) )
	{
		printf ( "ModelRenderer: failed to create descriptor set\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createPipelineLayout ( vkDev.device, descriptorSetLayout_, &pipelineLayout_ ) )
	{
		printf ( "ModelRenderer: failed to create pipeline layout\n" );
		exit ( EXIT_FAILURE );
	}

	std::vector<const char*> shaderFilenames = getShaderFilenamesWithRoot ( "assets/shaders/VK02.vert", "assets/shaders/VK02.frag", "assets/shaders/VK02.geom" );

	if ( !createGraphicsPipeline ( vkDev, renderPass_, pipelineLayout_, shaderFilenames, &graphicsPipeline_ ))
	{
		printf ( "ModelRenderer: failed to create graphics pipeline\n" );
		exit ( EXIT_FAILURE );
	}

}

bool ModelRenderer::createDescriptorSet ( VulkanRenderDevice& vkDev, uint32_t uniformDataSize )
{
	const std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
		descriptorSetLayoutBinding ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )
	};

	// This LayoutBinding description is used to create an immutable descriptor set layout that does not change later
	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size ()),
		.pBindings = bindings.data ()
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

	// for each swapchainImage, update the corresponding descriptorSet with specific buffer handles
	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		VkDescriptorSet ds = descriptorSets_[i];
		
		const VkDescriptorBufferInfo bufferInfo = {
			uniformBuffers_[i], 0, uniformDataSize
		};

		const VkDescriptorBufferInfo bufferInfo2 = {
			storageBuffer_, 0, vertexBufferSize_
		};

		const VkDescriptorBufferInfo bufferInfo3 = {
			storageBuffer_, vertexBufferSize_, indexBufferSize_
		};

		const VkDescriptorImageInfo imageInfo = {
			textureSampler_,
			texture_.imageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		const std::array<VkWriteDescriptorSet, 4> descriptorWrites = {
			bufferWriteDescriptorSet ( ds, &bufferInfo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ),
			bufferWriteDescriptorSet ( ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ),
			bufferWriteDescriptorSet ( ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ),
			imageWriteDescriptorSet ( ds, &imageInfo, 3 )
		};

		vkUpdateDescriptorSets ( vkDev.device, static_cast<uint32_t>(descriptorWrites.size ()), descriptorWrites.data (), 0, nullptr );
	}

	return true;
}

void ModelRenderer::updateUniformBuffer ( VulkanRenderDevice& vkDev, uint32_t currentImage, const void* data, const size_t dataSize )
{
	uploadBufferData ( vkDev, uniformBuffersMemory_[currentImage], 0, data, dataSize );
}

void ModelRenderer::fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage )
{
	beginRenderPass ( commandBuffer, currentImage );
	vkCmdDraw ( commandBuffer, indexBufferSize_ / (sizeof ( uint32_t )), 1, 0, 0 );
	vkCmdEndRenderPass ( commandBuffer );
}

ModelRenderer::~ModelRenderer ()
{
	vkDestroyBuffer ( device_, storageBuffer_, nullptr );
	vkFreeMemory ( device_, storageBufferMemory_, nullptr );

	vkDestroySampler ( device_, textureSampler_, nullptr );
	destroyVulkanImage ( device_, texture_ );

	destroyVulkanImage ( device_, depthTexture_ );
}