#ifndef __VULKAN_MULTI_MESH_RENDERER_H__
#define __VULKAN_MULTI_MESH_RENDERER_H__

// TODO: implement!

#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <jcCommonFramework/scene/VtxData.h>

using glm::mat4;

class MultiMeshRenderer : public RendererBase
{
public:
	uint32_t vertexBufferSize_;
	uint32_t indexBufferSize_;

private:
	VulkanRenderDevice& vkDev_;

	uint32_t maxVertexBufferSize_;
	uint32_t maxIndexBufferSize_;

	uint32_t maxShapes_;

	uint32_t maxDrawDataSize_;
	uint32_t maxMaterialSize_;

	VkBuffer storageBuffer_;
	VkDeviceMemory storageBufferMemory_;

	VkBuffer materialBuffer_;
	VkDeviceMemory materialBufferMemory_;

	std::vector<VkBuffer> indirectBuffers_;
	std::vector<VkDeviceMemory> indirectBuffersMemory_;

	std::vector<VkBuffer> drawDataBuffers_;
	std::vector<VkDeviceMemory> drawDataBuffersMemory_;

	std::vector<VkBuffer> countBuffers_;
	std::vector<VkDeviceMemory> countBuffersMemory_;

	std::vector<DrawData> shapes_;
	MeshData meshData_;

public:
	MultiMeshRenderer (
		VulkanRenderDevice& vkDev,
		const char* meshFile,
		const char* drawDataFile,
		const char* materialFile,
		const char* vtxShaderFile,
		const char* fragShaderFile );

	virtual ~MultiMeshRenderer ();

	void updateIndirectBuffers ( VulkanRenderDevice& vkDev, size_t currentImage, bool* visibility = nullptr );

	void updateGeometryBuffers ( VulkanRenderDevice& vkDev, uint32_t vertexCount, uint32_t indexCount, const void* vertices, const void* indices );
	void updateMaterialBuffer ( VulkanRenderDevice& vkDev, uint32_t materialSize, const void* materialData );

	void updateUniformBuffer ( VulkanRenderDevice& vkDev, size_t currentImage, const mat4& m );
	void updateDrawDataBuffer ( VulkanRenderDevice& vkDev, size_t currentImage, uint32_t drawDataSize, const void* drawData );
	void updateCountBuffer ( VulkanRenderDevice& vkDev, size_t currentImage, uint32_t itemCount );

	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;

private:

	bool createDescriptorSet ( VulkanRenderDevice& vkDev );
	void loadDrawData ( const char* drawDataFile );
};

#endif // !__VULKAN_MULTI_MESH_RENDERER_H__
