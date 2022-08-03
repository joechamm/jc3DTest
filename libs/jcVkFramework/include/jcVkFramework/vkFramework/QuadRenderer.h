#ifndef __QUAD_RENDERER_H__
#define __QUAD_RENDERER_H__

#include <jcVkFramework/vkFramework/Renderer.h>

// A companion for LineCanvas to render texture quadrangles
struct QuadRenderer : public Renderer
{
private:
	struct VertexData
	{
		glm::vec3 pos;
		glm::vec2 tc;
		int texIdx;
	};

	std::vector<VertexData> quads_;
	std::vector<VulkanBuffer> storages_;

public:
	QuadRenderer ( VulkanRenderContext& ctx, const std::vector<VulkanTexture>& textures, const std::vector<VulkanTexture>& outputs = {}, RenderPass screenRenderPass = RenderPass () );

	void fillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass renderPass = VK_NULL_HANDLE) override;
	void updateBuffers ( size_t currentImage ) override;

	void quad ( float x1, float y1, float x2, float y2, int texIdx );
	void clear ()
	{
		quads_.clear (); 
	}
};

#endif // !__QUAD_RENDERER_H__
