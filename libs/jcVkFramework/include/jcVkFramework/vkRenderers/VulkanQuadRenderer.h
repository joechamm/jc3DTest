#ifndef __VULKAN_QUAD_RENDERER_H__
#define __VULKAN_QUAD_RENDERER_H__

//#include "vkRenderers/VulkanRendererBase.h"
#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>

#include <string>
#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

class VulkanQuadRenderer : public RendererBase
{
private:
	struct ConstBuffer
	{
		vec2 offset;
		uint32_t textureIndex;
	};

	struct VertexData
	{
		vec3 pos;
		vec2 tc;
	};

	VulkanRenderDevice& vkDev_;

	std::vector<VertexData> quads_;

	size_t vertexBufferSize_;
	size_t indexBufferSize_;

	// Storage Buffer with index and vertex data
	std::vector<VkBuffer> storageBuffers_;
	std::vector<VkDeviceMemory> storageBuffersMemory_;

	std::vector<VulkanImage> textures_;
	std::vector<VkSampler> textureSamplers_;

public:
	VulkanQuadRenderer ( VulkanRenderDevice& vkDev, const std::vector<std::string>& textureFiles );
	virtual ~VulkanQuadRenderer ();

	void updateBuffer ( VulkanRenderDevice& vkDev, size_t i );
	void pushConstants ( VkCommandBuffer commandBuffer, uint32_t textureIndex, const vec2& offset );

	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;

	void quad ( float x1, float y1, float x2, float y2 );
	void clear ();

private:
	bool createDescriptorSet ( VulkanRenderDevice& vkDev );
};

#endif // !__VULKAN_QUAD_RENDERER_H__
