#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>

#include "Utils.h"
#include "UtilsVulkan.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <string>

#include <helpers/RootDir.h>

using std::string;
using glm::mat4;
using glm::vec4;
using glm::vec3;

const uint32_t kScreenWidth = 1280;
const uint32_t kScreenHeight = 720;

GLFWwindow* window;

struct UniformBuffer {
	mat4 mvp;
} ubo;

static constexpr VkClearColorValue clearValueColor = { 1.0f, 1.0f, 1.0f, 1.0f };

size_t vertexBufferSize;
size_t indexBufferSize;

VulkanInstance vk;
VulkanRenderDevice vkDev;

struct VulkanState {
	// 1. Descriptor set (layout + pool + sets) -> uses uniform buffers, textures, framebuffers
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	// 2.
	std::vector<VkFramebuffer> swapchainFramebuffers;

	// 3. Pipeline &  render pass (using DescriptorSets & pipeline state options)
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	// 4. Uniform buffer
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	// 5. Storage Buffer with index and vertex data
	VkBuffer storageBuffer;
	VkDeviceMemory storageBufferMemory;

	// 6. Depth buffer
//	VulkanImage depthTexture;

	VkSampler textureSampler;
//	VulkanImage texture;
} vkState;

bool createUniformBuffers ()
{
	VkDeviceSize bufferSize = sizeof ( UniformBuffer );
	vkState.uniformBuffers.resize ( vkDev.swapchainImages.size () );
	vkState.uniformBuffersMemory.resize ( vkDev.swapchainImages.size () );
	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		if ( !createBuffer (
			/* VkDevice device */					vkDev.device,
			/* VkPhysicalDevice physicalDevice */	vkDev.physicalDevice,
			/* VkDeviceSize size */					bufferSize,
			/* VkBufferUsageFlags usage */			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			/* VkMemoryPropertyFlags properties */	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			/* VkBuffer &buffer */					vkState.uniformBuffers[i],
			/* VkDeviceMemory &bufferMemory */		vkState.uniformBuffersMemory[i] ) )
		{
			printf ( "Fail: buffers\n" );
			return false;
		}		
	}

	return true;
}

void updateUniformBuffer ( uint32_t currentImage, const UniformBuffer& ubo )
{
	void* data = nullptr;
	vkMapMemory (
		/* VkDevice device */			vkDev.device, 
		/* VkDeviceMemory memory */		vkState.uniformBuffersMemory[currentImage], 
		/* VkDeviceSize offset */		0, 
		/* VkDeviceSize size */			sizeof(ubo), 
		/* VkMemoryMapFlags flags */	0, 
		/* void **ppData */				&data
	);
	memcpy ( data, &ubo, sizeof ( ubo ) );
	vkUnmapMemory ( vkDev.device, vkState.uniformBuffersMemory[currentImage] );
}

bool fillCommandBuffers ( size_t i )
{
	const VkCommandBufferBeginInfo bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	const std::array<VkClearValue, 2> clearValues = {
		VkClearValue { .color = clearValueColor }, 
		VkClearValue { .depthStencil = { 1.0f, 0 } } };

	const VkRect2D screenRect = { 
		.offset = { 0, 0 }, 
		.extent = { .width = kScreenWidth, .height = kScreenHeight }
	};
	
	VK_CHECK ( vkBeginCommandBuffer ( vkDev.commandBuffers[i], &bi ) );
	
	const VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = vkState.renderPass,
		.framebuffer = vkState.swapchainFramebuffers[i],
		.renderArea = screenRect,
		.clearValueCount = static_cast<uint32_t>(clearValues.size ()),
		.pClearValues = clearValues.data ()
	};

	vkCmdBeginRenderPass (
		/* VkCommandBuffer commandBuffer */	vkDev.commandBuffers[i],
		/* const VkRenderPassBeginInfo *pRenderPassBegin */ &renderPassInfo,
		/* VkSubpassContents contents */ VK_SUBPASS_CONTENTS_INLINE 
	);
		
	vkCmdBindPipeline (
		/* VkCommandBuffer commandBuffer */				vkDev.commandBuffers[i],
		/* VkPipelineBindPoint pipelineBindPoint */		VK_PIPELINE_BIND_POINT_GRAPHICS,
		/* VkPipeline pipeline */						vkState.graphicsPipeline 
	);

	vkCmdBindDescriptorSets (
		/* VkCommandBuffer commandBuffer */				vkDev.commandBuffers[i],
		/* VkPipelineBindPoint pipelineBindPoint */		VK_PIPELINE_BIND_POINT_GRAPHICS,
		/* VkPipelineLayout layout */					vkState.pipelineLayout,
		/* uint32_t firstSet */							0,
		/* uint32_t descriptorSetCount */				1,
		/* const VkDescriptorSet *pDescriptorSets */	&vkState.descriptorSets[i],
		/* uint32_t dynamicOffsetCount */				0,
		/* const uint32_t *pDynamicOffsets */			nullptr
	);

	vkCmdDraw (
		/* VkCommandBuffer commandBuffer */				vkDev.commandBuffers[i],
		/* uint32_t vertexCount */						static_cast<uint32_t>(indexBufferSize / sizeof(uint32_t)),
		/* uint32_t instanceCount */					1,
		/* uint32_t firstVertex */						0,
		/* uint32_t firstInstance*/						0
	);

	vkCmdEndRenderPass ( vkDev.commandBuffers[i] );

	VK_CHECK ( vkEndCommandBuffer ( vkDev.commandBuffers[i] ) );
	return true;
}
