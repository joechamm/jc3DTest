#ifndef __VULKAN_RENDERER_BASE_H__
#define __VULKAN_RENDERER_BASE_H__

#include <jcVkFramework/UtilsVulkan.h>

/*
	According to Chapter 04: Adding User Interaction and Productivity Tools of 3D Graphics Rendering Cookbook, we will assume that each frame is
	composed of multiple layers. The first is the solid background color. The next layer might be a 3D scene with models and lights. On top of that
	may be some wireframe meshes to draw debugging information. Another layer might be a 2D user interface such as ImGui. The finishing layer 
	transitions the swapchain image to the VK_LAYOUT_PRESENT_SRC_KHR lyaout. The RendererBase is an interface to render a single layer and implement
	this interface for screen-clearing and finishing layers.

	Each layer is an object that contains a Vulkan pipeline object, framebuffers, a Vulkan rendering pass, all descriptor sets, and all kinds of buffers
	necessary for rendering. This object provides an interface that can fill Vulkan command buffers for the current frame and update GPU buffers with
	CPU data, such as per-frame uniforms for camera transformation.
*/

class RendererBase
{
protected:
	VkDevice device_ = nullptr;

	uint32_t framebufferWidth_ = 0;
	uint32_t framebufferHeight_ = 0;

	// Descriptor set (layout + pool + sets) -> uses uniform buffers, textures, framebuffers
	// All textures and buffers are bound to the shader modules by descriptor sets. We maintain one 
	// descriptor set per swapchain image.
	VkDescriptorSetLayout descriptorSetLayout_ = nullptr;
	VkDescriptorPool descriptorPool_ = nullptr;
	std::vector<VkDescriptorSet> descriptorSets_;

	// Framebuffers (one for each command buffer)
	// Each command buffer operates on a dedicated framebuffer object.
	std::vector<VkFramebuffer> swapchainFramebuffers_;

	// 4. Pipeline & render pass (using DescriptorSets & pipeline state options)
	// We store the depth buffer reference, which is passed during the initialization phase.
	// The render pass, pipeline layout, and the pipeline itself are also taken from the VulkanState object.
	VkRenderPass renderPass_ = nullptr;
	VkPipelineLayout pipelineLayout_ = nullptr;
	VkPipeline graphicsPipeline_ = nullptr;
	VulkanImage depthTexture_;

	// 5. Uniform buffer
	// Each swapchain image has an associated uniform buffer.
	std::vector<VkBuffer> uniformBuffers_;
	std::vector<VkDeviceMemory> uniformBuffersMemory_;
public:
	explicit RendererBase ( const VulkanRenderDevice& vkDev, VulkanImage depthTexture )
		: device_ ( vkDev.device )
		, framebufferWidth_ ( vkDev.framebufferWidth )
		, framebufferHeight_ ( vkDev.framebufferHeight )
		, depthTexture_ ( depthTexture )
	{}

	virtual ~RendererBase ();

	/**
	* \brief Injects a stream of Vulkan commands
	*
	* \param commandBuffer the buffer to pass the commands to
	* \param currentImage index to the swapchain image
	*
	*/
	virtual void fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage ) = 0;

	/**
	* \brief give access to the internally managed depth buffer
	* 
	* \return depthTexture 
	*
	*/
	inline VulkanImage getDepthTexture () const
	{
		return depthTexture_;
	}

protected:
	/**
	* \brief begin render pass
	* 
	* emits the vkCmdBeginRenderPass, vkCmdBindPipeline, vkCmdBindDescriptorSet to begin rendering
	*
	* \param commandBuffer command buffer to use
	* \param currentImage swapchain image index to use
	*/
	void beginRenderPass ( VkCommandBuffer commandBuffer, size_t currentImage );
	
	/**
	* \brief create uniform buffers
	*
	* allocates a list of GPU buffers that contain uniform data, with one buffer per swapchain image
	* 
	* \param vkDev VulkanRenderDevice to use
	* \param uniformDataSize size of the buffer to create
	* 
	* \return true if buffers created successfully
	*/
	bool createUniformBuffers ( VulkanRenderDevice& vkDev, size_t uniformDataSize );
};

#endif // __VULKAN_RENDERER_BASE_H__