#ifndef __VULKAN_COMPUTED_ITEM_H__
#define __VULKAN_COMPUTED_ITEM_H__

#include <jcCommonFramework/Utils.h>
#include <jcVkFramework/UtilsVulkan.h>

struct ComputedItem
{
protected:
	VulkanRenderDevice& vkDev_;

	VkFence fence_;

	VulkanBuffer uniformBuffer_;

	VkDescriptorSetLayout dsLayout_;
	VkDescriptorPool descriptorPool_;
	VkDescriptorSet descriptorSet_;
	VkPipelineLayout pipelineLayout_;
	VkPipeline pipeline_;

public:
	ComputedItem ( VulkanRenderDevice& vkDev, uint32_t uniformBufferSize );
	virtual ~ComputedItem ();

	void fillComputeComandBuffer ( void* pushConstant = nullptr, uint32_t pushConstantSize = 0, uint32_t xsize = 1, uint32_t ysize = 1, uint32_t zsize = 1 );

	bool submit ();

	void waitFence ( void );

	inline void uploadUniformBuffer ( uint32_t size, void* data )
	{
		uploadBufferData ( vkDev_, uniformBuffer_.memory, 0, data, size );
	}
};

#endif // !__VULKAN_COMPUTED_ITEM_H__


