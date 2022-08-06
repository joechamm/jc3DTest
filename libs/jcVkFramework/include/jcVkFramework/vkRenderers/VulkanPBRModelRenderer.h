#ifndef __VULKAN_PBR_MODEL_RENDERER_H__
#define __VULKAN_PBR_MODEL_RENDERER_H__

// TODO: implement!
#include <jcVkFramework/vkRenderers/VulkanRendererBase.h>
#include <string>

class PBRModelRenderer : public RendererBase
{
	size_t vertexBufferSize_;
	size_t indexBufferSize_;

	VkBuffer storageBuffer_;
	VkDeviceMemory storageBufferMemory_;

	VulkanTexture texAO_;
	VulkanTexture texEmissive_;
	VulkanTexture texAlbedo_;
	VulkanTexture texMeR_;
	VulkanTexture texNormal_;

	VulkanTexture envMapIrradiance_;
	VulkanTexture envMap_;

	VulkanTexture brdfLUT_;

public:
	PBRModelRenderer ( VulkanRenderDevice& vkDev,
		uint32_t uniformBufferSize,
		const std::string& modelFile,
		const std::string& texAOFile,
		const std::string& texEmissiveFile,
		const std::string& texAlbedoFile,
		const std::string& texMeRFile,
		const std::string& texNormalFile,
		const std::string& texEnvMapFile,
		const std::string& texIrrMapFile,
		VulkanImage depthTexture );

	virtual ~PBRModelRenderer ();

	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) override;

	void updateUniformBuffer ( VulkanRenderDevice& vkDev, uint32_t currentImage, const void* data, const size_t dataSize );

private:
	bool createDescriptorSet ( VulkanRenderDevice& vkDev, uint32_t uniformDataSize );
};

#endif // !__VULKAN_PBR_MODEL_RENDERER_H__
