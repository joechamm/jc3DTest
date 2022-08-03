#include <jcVkFramework/vkFramework/InfinitePlaneRenderer.h>
#include <jcVkFramework/ResourceString.h>

InfinitePlaneRenderer::InfinitePlaneRenderer ( VulkanRenderContext& ctx, const std::vector<VulkanTexture>& outputs, RenderPass screenRenderPass )
 : Renderer ( ctx )
{
	const PipelineInfo pInfo = initRenderPass ( PipelineInfo{}, outputs, screenRenderPass, ctx.screenRenderPass_NoDepth_ );

	const size_t imgCount = ctx.vkDev_.swapchainImages.size ();
	descriptorSets_.resize ( imgCount );
	uniforms_.resize ( imgCount );

	DescriptorSetInfo dsInfo = {
		.buffers_ = {
			uniformBufferAttachment ( VulkanBuffer{}, 0, sizeof ( UniformBuffer ), VK_SHADER_STAGE_VERTEX_BIT )
		}
	};

	descriptorSetLayout_ = ctx_.resources_.addDescriptorSetLayout ( dsInfo );
	descriptorPool_ = ctx_.resources_.addDescriptorPool ( dsInfo, imgCount );

	for ( size_t i = 0; i < imgCount; i++ )
	{
		uniforms_[i] = ctx.resources_.addUniformBuffer ( sizeof ( UniformBuffer ) );
		dsInfo.buffers_[0].buffer_ = uniforms_[i];

		descriptorSets_[i] = ctx.resources_.addDescriptorSet ( descriptorPool_, descriptorSetLayout_ );
		ctx.resources_.updateDescriptorSet ( descriptorSets_[i], dsInfo );		
	}

	initPipeline ( getShaderFilenamesWithRoot ( "assets/shaders/VK07_InfinitePlane.vert", "assets/shaders/VK07_InfinitePlane.frag" ), pInfo );
}

void InfinitePlaneRenderer::fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp )
{
	beginRenderPass ( (rp != VK_NULL_HANDLE) ? rp : renderPass_.handle_, (fb != VK_NULL_HANDLE) ? fb : framebuffer_, cmdBuffer, currentImage );

	vkCmdDraw ( cmdBuffer, 6, 1, 0, 0 );
	vkCmdEndRenderPass ( cmdBuffer );
}

void InfinitePlaneRenderer::updateBuffers ( size_t currentImage )
{
	const UniformBuffer ubo = {
		.proj = proj_,
		.view = view_,
		.model = model_,
		.time = (float)glfwGetTime ()
	};
	uploadBufferData ( ctx_.vkDev_, uniforms_[currentImage].memory, 0, &ubo, sizeof ( ubo ) );
}
