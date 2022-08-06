#ifndef __VULKAN_COMPUTED_VB_H__
#define __VULKAN_COMPUTED_VB_H__

#include <jcCommonFramework/Utils.h>
#include <jcVkFramework/vkRenderers/VulkanComputedItem.h>

struct ComputedVertexBuffer : public ComputedItem
{
	VkBuffer computedBuffer_;
	VkDeviceMemory computedMemory_;

	uint32_t computedVertexCount_;
protected:
	uint32_t indexBufferSize_;
	uint32_t vertexSize_;

	bool canDownloadVertices_;

public:
	ComputedVertexBuffer ( VulkanRenderDevice& vkDev, const char* shaderName, uint32_t indexBufferSize, uint32_t uniformBufferSize, uint32_t vertexSize, uint32_t vertexCount, bool supportDownload = false );
	virtual ~ComputedVertexBuffer ()
	{}

	void uploadIndexData ( uint32_t* indices );

	void downloadVertices ( void* vertexData );

protected:
	bool createComputedBuffer ();
	bool createDescriptorSet ();
	bool createComputedSetLayout ();
};

#endif // !__VULKAN_COMPUTED_ITEM_H__


