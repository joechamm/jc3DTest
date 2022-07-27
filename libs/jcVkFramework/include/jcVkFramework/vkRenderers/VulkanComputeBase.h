#ifndef __VULKAN_COMPUTE_BASE_H__
#define __VULKAN_COMPUTE_BASE_H__

#include <jcVkFramework/Utils.h>
#include <jcVkFramework/UtilsVulkan.h>

/* 
*	To manipulate data buffers on the GPU and use the data, we need three basic operations: upload data from the host memory into a GPU buffer, download data from a GPU buffer to the host memory, and run a compute shader workload on that buffer. 
*	The data uploading and downloading process consists of mapping the GPU memory to the host address space and then using memcpy() to transfer buffer contents
*/

struct ComputeBase
{
protected:
	VulkanRenderDevice& vkDev_;

	VkBuffer inBuffer_;
	VkBuffer outBuffer_;
	VkDeviceMemory inBufferMemory_;
	VkDeviceMemory outBufferMemory_;

	VkDescriptorSetLayout dsLayout_;
	VkPipelineLayout pipelineLayout_;
	VkPipeline pipeline_;

	VkDescriptorPool descriptorPool_;
	VkDescriptorSet descriptorSet_;

public:
	ComputeBase ( VulkanRenderDevice& vkDev, const char* shaderName, uint32_t inputSize, uint32_t outputSize );

	virtual ~ComputeBase ();

	inline void uploadInput ( uint32_t offset, void* inData, uint32_t byteCount )
	{
		uploadBufferData ( vkDev_, inBufferMemory_, offset, inData, byteCount );
	}

	inline void downloadOutput ( uint32_t offset, void* outData, uint32_t byteCount )
	{
		downloadBufferData ( vkDev_, outBufferMemory_, offset, outData, byteCount );
	}

	inline bool execute ( uint32_t xsize, uint32_t ysize, uint32_t zsize )
	{
		return executeComputeShader ( vkDev_, pipeline_, pipelineLayout_, descriptorSet_, xsize, ysize, zsize );
	}

protected:

	bool createComputeDescriptorSet ( VkDevice device, VkDescriptorSetLayout descriptorSetLayout );
};

#endif // !__VULKAN_COMPUTE_BASE_H__
