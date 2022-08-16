#ifndef __CUBE_RENDERER_H__
#define __CUBE_RENDERER_H__

#include <jcVkFramework/vkFramework/Renderer.h>

using glm::mat4;

struct CubemapRenderer : public Renderer
{
private:
	struct UniformBuffer
	{
		mat4 proj_;
		mat4 view_;
	} ubo_;

public:
	CubemapRenderer ( VulkanRenderContext& ctx,
		VulkanTexture texture,
		const std::vector<VulkanTexture>& outputs = std::vector<VulkanTexture> {},
		RenderPass screenRenderPass = RenderPass () );

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE ) override;

	void updateBuffers ( size_t currentImage ) override;

	inline void setMatrices ( const mat4& proj, const mat4& view )
	{
		ubo_.proj_ = proj;
		ubo_.view_ = view;
	}
};

#endif // __CUBE_RENDERER_H__