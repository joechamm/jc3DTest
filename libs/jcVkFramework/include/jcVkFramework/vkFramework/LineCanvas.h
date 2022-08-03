#ifndef __LINE_CANVAS_H__
#define __LINE_CANVAS_H__

#include <jcVkFramework/vkFramework/Renderer.h>

struct LineCanvas : public Renderer
{
private:
	glm::mat4 mvp_;

	struct UniformBuffer
	{
		glm::mat4 mvp;
		float time;
	};

	struct VertexData
	{
		glm::vec3 position;
		glm::vec4 color;
	};
	
	std::vector<VertexData> lines_;

	// Storage Buffer with index and vertex data
	std::vector<VulkanBuffer> storages_;

	static constexpr unsigned kMaxLinesCount = 4 * 8 * 65536;
	static constexpr unsigned kMaxLinesDataSize = kMaxLinesCount * sizeof ( LineCanvas::VertexData ) * 2;
public:

	explicit LineCanvas ( VulkanRenderContext& ctx,
		bool useDepth = true,
		const std::vector<VulkanTexture>& outputs = std::vector<VulkanTexture>{},
		VkFramebuffer framebuffer = VK_NULL_HANDLE,
		RenderPass screenRenderPass = RenderPass () );

	/* Strictly offscreen rendering */
	explicit LineCanvas(VulkanRenderContext& ctx, const std::vector<VulkanTexture>& outputs, bool firstPass = false) :
		LineCanvas ( ctx, true, outputs, VK_NULL_HANDLE, ctx.resources_.addRenderPass ( outputs, RenderPassCreateInfo { .clearColor_ = firstPass, .clearDepth_ = firstPass , .flags_ = (uint8_t ( (firstPass ? eRenderPassBit_First : eRenderPassBit_Offscreen) | eRenderPassBit_Offscreen )) }) )
	{}

	void fillCommandBuffer(VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override;
	void updateBuffers ( size_t currentImage ) override;

	void clear ()
	{
		lines_.clear ();
	}

	void line ( const vec3& p1, const vec3& p2, const vec4& c );
	void plane3d(const vec3& orig, const vec3& v1, const vec3& v2, int n1, int n2, float s1, float s2, const vec4& color, const vec4& outlineColor);
	
	inline void setCameraMatrix(const glm::mat4& mvp) { mvp_ = mvp; }
};

void drawBox3d(LineCanvas& canvas, const glm::mat4& m, const BoundingBox& box, const glm::vec4& color);
void renderCameraFrustrum(LineCanvas& canvas, const glm::mat4& camView, const glm::mat4& camProj, const glm::vec4& camColor);
#endif // !__LINE_CANVAS_H__
