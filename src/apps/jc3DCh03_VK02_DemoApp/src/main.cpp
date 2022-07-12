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
	VulkanTexture texture;
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

// the descriptor set must have a fixed layout that describes the number and usage type of all the texture samples and buffers
bool createDescriptorSet ()
{
	// Declare a list of buffers and sampler descriptions. Each entry defines which shader unit it is bound to, the exact data type, and which shader stage can access it.
	const std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
		descriptorSetLayoutBinding ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )
	};

	const VkDescriptorSetLayoutCreateInfo li = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size ()),
		.pBindings = bindings.data ()
	};

	VK_CHECK ( vkCreateDescriptorSetLayout ( vkDev.device, &li, nullptr, &vkState.descriptorSetLayout ) );

	// allocate a number of descriptor set layouts, one for each swap chain image, just like we did with the uniform and command buffers
	std::vector<VkDescriptorSetLayout> layouts ( vkDev.swapchainImages.size (), vkState.descriptorSetLayout );
	VkDescriptorSetAllocateInfo ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = vkState.descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(vkDev.swapchainImages.size ()),
		.pSetLayouts = layouts.data ()
	};
	vkState.descriptorSets.resize ( vkDev.swapchainImages.size () );
	VK_CHECK ( vkAllocateDescriptorSets ( vkDev.device, &ai, vkState.descriptorSets.data () ) );

	// Once we have allocated the descriptor sets with the specified layout, we must update these descriptor sets with concreate buffer and texture handles.
	// This operation can be viewed as an analogue of texture and buffer binding in OpenGL. The crucial difference is that we do not do this at every frame since
	// binding is prebaked into the pipeline. The minor downside of this approach is that we cannot simply change the texture from frame to frame.
	// For this example we will use one uniform buffer, one index buffer, one vertex buffer, and one texture
	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		VkDescriptorBufferInfo bufferInfo = {
			.buffer = vkState.uniformBuffers[i],
			.offset = 0,
			.range = sizeof ( UniformBuffer )
		};

		VkDescriptorBufferInfo bufferInfo2 = {
			.buffer = vkState.storageBuffer,
			.offset = 0,
			.range = vertexBufferSize
		};
		VkDescriptorBufferInfo bufferInfo3 = {
			.buffer = vkState.storageBuffer,
			.offset = vertexBufferSize,
			.range = indexBufferSize
		};
		VkDescriptorImageInfo imageInfo = {
			.sampler = vkState.textureSampler,
			.imageView = vkState.texture.imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		// The VkWriteDescriptorSet operation array contains all the "bindings" for the buffers we declared previously
		std::array<VkWriteDescriptorSet, 4> descriptorWrites = {
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vkState.descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &bufferInfo
				},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vkState.descriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &bufferInfo2
				},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vkState.descriptorSets[i],
				.dstBinding = 2,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &bufferInfo3
				},
			VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vkState.descriptorSets[i],
				.dstBinding = 3,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfo
				},
		};

		// update the descriptor by applying the necessary descripor write operations
		vkUpdateDescriptorSets ( vkDev.device, static_cast<uint32_t>(descriptorWrites.size ()), descriptorWrites.data (), 0, nullptr );
	}

	return true;
}