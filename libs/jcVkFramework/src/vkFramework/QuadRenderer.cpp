#include <jcVkFramework/vkFramework/QuadRenderer.h>
#include <jcCommonFramework/ResourceString.h>

static constexpr int MAX_QUADS = 256;

QuadRenderer::QuadRenderer ( VulkanRenderContext& ctx, const std::vector<VulkanTexture>& textures, const std::vector<VulkanTexture>& outputs, RenderPass screenRenderPass )
	: Renderer ( ctx )
{
	const PipelineInfo pInfo = initRenderPass ( PipelineInfo{}, outputs, screenRenderPass, ctx.screenRenderPass_NoDepth_ );

	uint32_t vertexBufferSize = MAX_QUADS * 6 * sizeof ( VertexData );
	
	const size_t imgCount = ctx.vkDev_.swapchainImages.size ();
	descriptorSets_.resize ( imgCount );
	storages_.resize ( imgCount );

	DescriptorSetInfo dsInfo = {
		.buffers_ = {
			storageBufferAttachment ( VulkanBuffer {}, 0, vertexBufferSize, VK_SHADER_STAGE_VERTEX_BIT ) },
		.textureArrays_ = {
			fsTextureArrayAttachment ( textures ) }
	};

	descriptorSetLayout_ = ctx.resources_.addDescriptorSetLayout ( dsInfo );
	descriptorPool_ = ctx.resources_.addDescriptorPool ( dsInfo, imgCount );

	for ( size_t i = 0; i < imgCount; i++ )
	{
		storages_[i] = ctx.resources_.addStorageBuffer (vertexBufferSize );
		dsInfo.buffers_[0].buffer_ = storages_[i];
		descriptorSets_[i] = ctx.resources_.addDescriptorSet ( descriptorPool_, descriptorSetLayout_ );
		ctx.resources_.updateDescriptorSet ( descriptorSets_[i], dsInfo );
	}
	
	initPipeline ( getShaderFilenamesWithRoot ( "assets/shaders/VK07_QuadRenderer.vert", "assets/shaders/VK07_QuadRenderer.frag" ), pInfo );
}

void QuadRenderer::fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp )
{
	if ( quads_.empty () ) return;

	beginRenderPass ( (rp != VK_NULL_HANDLE) ? rp : renderPass_.handle_, (fb != VK_NULL_HANDLE) ? fb : framebuffer_, cmdBuffer, currentImage);
	vkCmdDraw ( cmdBuffer, static_cast<uint32_t>(quads_.size ()), 1, 0, 0 );
	vkCmdEndRenderPass ( cmdBuffer );
}

void QuadRenderer::updateBuffers ( size_t currentImage )
{
	if ( !quads_.empty () ) uploadBufferData ( ctx_.vkDev_, storages_[currentImage].memory, 0, quads_.data (), quads_.size () * sizeof ( VertexData ) );
}

void QuadRenderer::quad(float x1, float y1, float x2, float y2, int texIdx) 
{
	VertexData v1 { { x1, y1, 0.0f }, { 0.0f, 0.0f }, texIdx };
	VertexData v2 { { x2, y1, 0.0f }, { 1.0f, 0.0f }, texIdx };
	VertexData v3 { { x2, y2, 0.0f }, { 1.0f, 1.0f }, texIdx };
	VertexData v4 { { x1, y2, 0.0f }, { 0.0f, 1.0f }, texIdx };
	quads_.push_back ( v1 );
	quads_.push_back ( v2 );
	quads_.push_back ( v3 );
	quads_.push_back ( v1 );
	quads_.push_back ( v3 );
	quads_.push_back ( v4 );
}
