#include <jcVkFramework/vkFramework/CubeRenderer.h>
#include <jcCommonFramework/ResourceString.h>

CubemapRenderer::CubemapRenderer ( VulkanRenderContext& ctx, VulkanTexture texture, const std::vector<VulkanTexture>& outputs, RenderPass screenRenderPass )
	: Renderer(ctx)
{
	const PipelineInfo pInfo = initRenderPass ( PipelineInfo {}, outputs, screenRenderPass, ctx.screenRenderPass_ );

	const size_t imgCount = ctx.vkDev_.swapchainImages.size ();
	descriptorSets_.resize ( imgCount );
	uniforms_.resize ( imgCount );

	DescriptorSetInfo dsInfo = {
		.buffers_ = { uniformBufferAttachment ( VulkanBuffer{}, 0, sizeof ( UniformBuffer ), VK_SHADER_STAGE_VERTEX_BIT ) },
		.textures_ = { fsTextureAttachment ( texture ) }
	};

	descriptorSetLayout_ = ctx_.resources_.addDescriptorSetLayout ( dsInfo );
	descriptorPool_ = ctx_.resources_.addDescriptorPool ( dsInfo, imgCount );

	for ( size_t i = 0; i != imgCount; i++ )
	{
		uniforms_ [ i ] = ctx_.resources_.addUniformBuffer ( sizeof ( UniformBuffer ) );
		dsInfo.buffers_ [ 0 ].buffer_ = uniforms_ [ i ];

		descriptorSets_ [ i ] = ctx_.resources_.addDescriptorSet ( descriptorPool_, descriptorSetLayout_ );
		ctx_.resources_.updateDescriptorSet ( descriptorSets_ [ i ], dsInfo );
	}

	initPipeline ( getShaderFilenamesWithRoot ( "assets/shaders/VK10_CubeMap.vert", "assets/shaders/VK10_CubeMap.frag" ), pInfo );
}

void CubemapRenderer::updateBuffers ( size_t currentImage )
{
	updateUniformBuffer ( currentImage, 0, sizeof ( UniformBuffer ), &ubo_ );
}

void CubemapRenderer::fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp )
{
	beginRenderPass ( ( rp != VK_NULL_HANDLE ) ? rp : renderPass_.handle_, ( fb != VK_NULL_HANDLE ) ? fb : framebuffer_, cmdBuffer, currentImage );

	vkCmdDraw ( cmdBuffer, 36, 1, 0, 0 );
	vkCmdEndRenderPass ( cmdBuffer );
}