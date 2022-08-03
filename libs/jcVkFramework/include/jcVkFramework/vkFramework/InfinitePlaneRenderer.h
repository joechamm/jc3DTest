#ifndef __INFINITE_PLANE_RENDERER_H__
#define __INFINITE_PLANE_RENDERER_H__

#include <jcVkFramework/vkFramework/Renderer.h>

// A port of GL infinite plane
struct InfinitePlaneRenderer : public Renderer
{
private:
	glm::mat4 proj_;
	glm::mat4 view_;
	glm::mat4 model_;
	
	struct UniformBuffer
	{
		glm::mat4 proj;
		glm::mat4 view;
		glm::mat4 model;
		float time;
	};

public:
	InfinitePlaneRenderer ( VulkanRenderContext& ctx,
		const std::vector<VulkanTexture>& outputs = std::vector<VulkanTexture>{},
		RenderPass screenRenderPass = RenderPass () );

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE ) override;
	void updateBuffers(size_t currentImage) override;
	
	inline void setMatrices ( const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model )
	{
		proj_ = proj; 
		view_ = view;
		model_ = model;
	}	
};
#endif // !__INFINITE_PLANE_RENDERER_H__

