#include "vkRenderers/VulkanCube.h"
#include "EasyProfilerWrapper.h"
#include "ResourceString.h"

using glm::mat4;
using std::string;

bool CubeRenderer::createDescriptorSet ( VulkanRenderDevice& vkDev )
{
	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
		descriptorSetLayoutBinding ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )
	};

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

	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		VkDescriptorSet ds = descriptorSets_[i];

		const VkDescriptorBufferInfo bufferInfo = {
			uniformBuffers_[i], 0, sizeof ( glm::mat4 )
		};

		const VkDescriptorImageInfo imageInfo = {
			textureSampler_,
			texture_.imageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		const std::array<VkWriteDescriptorSet, 2> descriptorWrites = {
			bufferWriteDescriptorSet ( ds, &bufferInfo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ),
			imageWriteDescriptorSet ( ds, &imageInfo, 1 )
		};

		vkUpdateDescriptorSets ( vkDev.device, static_cast<uint32_t>(descriptorWrites.size ()), descriptorWrites.data (), 0, nullptr );
	}

	return true;
}

void CubeRenderer::fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage )
{
	EASY_FUNCTION ();

	beginRenderPass ( commandBuffer, currentImage );

	vkCmdDraw ( commandBuffer, 36, 1, 0, 0 );

	vkCmdEndRenderPass ( commandBuffer );
}

void CubeRenderer::updateUniformBuffer ( VulkanRenderDevice& vkDev, uint32_t currentImage, const glm::mat4& m )
{
	uploadBufferData ( vkDev, uniformBuffersMemory_[currentImage], 0, glm::value_ptr ( m ), sizeof ( glm::mat4 ) );
}

CubeRenderer::CubeRenderer ( VulkanRenderDevice& vkDev, VulkanImage inDepthTexture, const char* textureFile ) : RendererBase ( vkDev, inDepthTexture )
{
	// Resource loading
	createCubeTextureImage ( vkDev, textureFile, texture_.image, texture_.imageMemory );

	createImageView ( vkDev.device, texture_.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &texture_.imageView, VK_IMAGE_VIEW_TYPE_CUBE, 6 );
	createTextureSampler ( vkDev.device, &textureSampler_ );

	// Pipeline initialization
	if ( !createColorAndDepthRenderPass ( vkDev, true, &renderPass_, RenderPassCreateInfo () ) )
	{
		printf ( "CubeRenderer: failed to create color and depth render pass\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createUniformBuffers ( vkDev, sizeof ( glm::mat4 ) ) )
	{
		printf ( "CubeRenderer: failed to create uniform buffers\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createColorAndDepthFramebuffers ( vkDev, renderPass_, depthTexture_.imageView, swapchainFramebuffers_ ) )
	{
		printf ( "CubeRenderer: failed to create color framebuffers\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createDescriptorPool ( vkDev, 1, 0, 1, &descriptorPool_ ) )
	{
		printf ( "CubeRenderer: failed to create descriptor pool\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createDescriptorSet ( vkDev ) )
	{
		printf ( "CubeRenderer: failed to create descriptor set\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createPipelineLayout ( vkDev.device, descriptorSetLayout_, &pipelineLayout_ ) )
	{
		printf ( "CubeRenderer: failed to create pipeline layout\n" );
		exit ( EXIT_FAILURE );
	}

	std::vector<string> shaderFilenames = getShaderFilenamesWithRoot ( "assets/shaders/VKCube.vert", "assets/shaders/VKCube.frag" );

	if ( !createGraphicsPipeline ( vkDev, renderPass_, pipelineLayout_, shaderFilenames, &graphicsPipeline_ ) )
	{
		printf ( "CubeRenderer: failed to create graphics pipeline\n" );
		exit ( EXIT_FAILURE );
	}
}

CubeRenderer::~CubeRenderer ()
{
	vkDestroySampler ( device_, textureSampler_, nullptr );
	destroyVulkanImage ( device_, texture_ );
}