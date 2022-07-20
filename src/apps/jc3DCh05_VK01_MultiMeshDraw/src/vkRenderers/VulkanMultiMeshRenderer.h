#ifndef __VULKAN_MULTI_MESH_RENDERER_H__
#define __VULKAN_MULTI_MESH_RENDERER_H__

#include "vkRenderers/VulkanRendererBase.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>
using glm::mat4;

#include "VtxData.h"

/**
 * @brief MultiMeshRenderer takes in the names of shader files to render the meshes and filenames with input data. To render multiple meshes, we need instance data for each mesh, the material descriptoion, and the mesh geometry itself.
*/

class MultiMeshRenderer : public RendererBase
{
private:
	/* Mesh data */
	std::vector<InstanceData>	instances_;
	std::vector<Mesh>			meshes_;
	std::vector<uint32_t>		indexData_;
	std::vector<float>			vertexData_;

	/* Vulkan */
	VulkanRenderDevice&			vkDev_;

	VkBuffer					storageBuffer_;
	VkDeviceMemory				storageBufferMemory_;

	VkBuffer					materialBuffer_;
	VkDeviceMemory				materialBufferMemory_;

	/* For each of the swapchain images, we declare a copy of indirect rendering data and buffers for instance data */
	std::vector<VkBuffer>		indirectBuffers_;
	std::vector<VkDeviceMemory>	indirectBuffersMemory_;
	std::vector<VkBuffer>		instanceBuffers_;
	std::vector<VkDeviceMemory>	instanceBuffersMemory_;

	/* Cached buffer sizes */
	uint32_t					maxVertexBufferSize_;
	uint32_t					maxIndexBufferSize_;
	uint32_t					maxInstances_;
	uint32_t					maxInstanceSize_;
	uint32_t					maxMaterialSize_;


public:
	MultiMeshRenderer ( VulkanRenderDevice& vkDev, const char* meshFilename, const char* instanceFilename, const char* materialFilename, const char* vertShaderFilename, const char* fragShaderFilename );
	virtual ~MultiMeshRenderer ();

	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;

	void updateUniformBuffer ( VulkanRenderDevice& vkDev, size_t currentImage, const mat4& m );
	void updateInstanceBuffer ( VulkanRenderDevice& vkDev, size_t currentImage, uint32_t instanceSize, const void* instanceData );
	void updateGeometryBuffers ( VulkanRenderDevice& vkDev, uint32_t vertexCount, uint32_t indexCount, const void* vertices, const void* indices );
	void updateIndirectBuffers ( VulkanRenderDevice& vkDev, size_t currentImage );

private:
	bool createDescriptorSet ( VulkanRenderDevice& vkDev );
	void loadInstanceData ( const char* instanceFilename );
	MeshFileHeader loadMeshData ( const char* meshFilename );

};

#endif // __VULKAN_MULTI_MESH_RENDERER_H__