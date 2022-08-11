#ifndef __VULKAN_SHADER_PROCESSOR_H__
#define __VULKAN_SHADER_PROCESSOR_H__

#include <jcVkFramework/vkFramework/Renderer.h>
#include <jcCommonFramework/ResourceString.h>

/*
   @brief Shader (post)processor for fullscreen effects
   Multiple input textures, single output [color + depth]. Possibly, can be extented to multiple outputs (allocate appropriate framebuffer)
*/

struct VulkanShaderProcessor : public Renderer
{
private:
	uint32_t indexBufferSize_;

public:
	VulkanShaderProcessor ( VulkanRenderContext& ctx,
		const PipelineInfo& pInfo,
		const DescriptorSetInfo& dsInfo,
		const std::vector<std::string>& shaderFiles,
		const std::vector<VulkanTexture>& outputs,
		uint32_t indexBufferSize = 6 * 4,
		RenderPass screenRenderPass = RenderPass () );

	void fillCommandBuffer(VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override;
};

struct QuadProcessor : public VulkanShaderProcessor
{
	QuadProcessor ( VulkanRenderContext& ctx,
		const DescriptorSetInfo& dsInfo,
		const std::vector<VulkanTexture>& outputs,
		const char* shaderFile ) : VulkanShaderProcessor ( ctx, ctx.pipelineParametersForOutputs ( outputs ), dsInfo, std::vector<std::string> { appendToRoot ( "assets/shaders/VK08_Quad.vert" ), std::string ( shaderFile ) },
			outputs, 6 * 4, outputs.empty () ? ctx.screenRenderPass_ : RenderPass () )
	{
	}
};

struct BufferProcessor : public VulkanShaderProcessor
{
	BufferProcessor(VulkanRenderContext& ctx, const DescriptorSetInfo& dsInfo,
		const std::vector<VulkanTexture>& outputs, const std::vector<std::string>& shaderFiles,
		uint32_t indexBufferSize = 6 * 4, RenderPass renderPass = RenderPass()) :
			VulkanShaderProcessor(ctx, ctx.pipelineParametersForOutputs(outputs), dsInfo,
				shaderFiles, outputs, indexBufferSize, outputs.empty() ? ctx.screenRenderPass_ : renderPass) {}
};

struct OffscreenMeshRenderer : public BufferProcessor
{
	OffscreenMeshRenderer (
		VulkanRenderContext& ctx,
		VulkanBuffer uniformBuffer,
		const std::pair<BufferAttachment, BufferAttachment>& meshBuffer,
		const std::vector<TextureAttachment>& usedTextures,
		const std::vector<VulkanTexture>& outputs,
		const std::vector<std::string>& shaderFiles,
		bool firstPass = false ) : BufferProcessor ( ctx, 
			DescriptorSetInfo {
				.buffers_ = {
					uniformBufferAttachment(uniformBuffer, 0, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
					meshBuffer.first,
					meshBuffer.second
				},
				.textures_ = usedTextures
			},
			outputs, shaderFiles, meshBuffer.first.size_,
			ctx.resources_.addRenderPass ( outputs, RenderPassCreateInfo { .clearColor_ = firstPass, .clearDepth_ = firstPass, .flags_ = ( uint8_t ) ( ( firstPass ? eRenderPassBit_First : eRenderPassBit_OffscreenInternal ) | eRenderPassBit_Offscreen ) } ) 
		) {}
};
#endif // !__VULKAN_SHADER_PROCESSOR_H__
