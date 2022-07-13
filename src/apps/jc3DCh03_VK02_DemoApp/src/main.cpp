#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <string>

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
	VulkanTexture depthTexture;

	VkSampler textureSampler;
//	VulkanImage texture;
	VulkanTexture texture;

	// try attaching shader modules to state
	ShaderModule vertShader;
	ShaderModule geomShader;
	ShaderModule fragShader;
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

bool initVulkan ()
{
	// creat vulkan instance and set up debugging callbacks
	createInstance ( &vk.instance );
	if ( !setupDebugCallbacks ( vk.instance, &vk.messenger, &vk.reportCallback ) )
	{
		exit ( EXIT_FAILURE );
	}

	// create a window surface attached to the GLFW window and our Vulkan instance
	if ( glfwCreateWindowSurface ( vk.instance, window, nullptr, &vk.surface ) )
	{
		exit ( EXIT_FAILURE );
	}

	// initialize Vulkan objects
	if ( !initVulkanRenderDevice ( vk, vkDev, kScreenWidth, kScreenHeight, isDeviceSuitable, { .geometryShader = VK_TRUE } ) )
	{
		exit ( EXIT_FAILURE );
	}

	VK_CHECK ( createShaderModule ( vkDev.device, &vkState.vertShader, string ( ROOT_DIR ).append ( "assets/shaders/VK02.vert" ).c_str() ) );
	VK_CHECK ( createShaderModule ( vkDev.device, &vkState.geomShader, string ( ROOT_DIR ).append ( "assets/shaders/VK02.geom" ).c_str () ) );
	VK_CHECK ( createShaderModule ( vkDev.device, &vkState.fragShader, string ( ROOT_DIR ).append ( "assets/shaders/VK02.frag" ).c_str () ) );

	// load the rubber duck 3D model into a shader storage buffer
	if ( !createTexturedVertexBuffer ( vkDev, string ( ROOT_DIR ).append ( "assets/models/rubber_duck/scene.gltf" ).c_str (), &vkState.storageBuffer, &vkState.storageBufferMemory, &vertexBufferSize, &indexBufferSize ) || !createUniformBuffers () )
	{
		printf ( "Cannot create data buffers\n" );
		fflush ( stdout );
		exit ( EXIT_FAILURE );
	}

	// Initialize the pipeline shader stages using the shader modules we created
	const std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
		shaderStageInfo ( VK_SHADER_STAGE_VERTEX_BIT, vkState.vertShader, "main" ),
		shaderStageInfo ( VK_SHADER_STAGE_GEOMETRY_BIT, vkState.geomShader, "main" ),
		shaderStageInfo ( VK_SHADER_STAGE_FRAGMENT_BIT, vkState.fragShader, "main" )
	};

	// load the duck texture image and create an image view with a sampler
	createTextureImage ( vkDev, string ( ROOT_DIR ).append ( "assets/models/rubber_duck/textures/Duck_baseColor.png" ).c_str (), vkState.texture.image, vkState.texture.imageMemory );
	createImageView ( vkDev.device, vkState.texture.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &vkState.texture.imageView );
	createTextureSampler ( vkDev.device, &vkState.textureSampler );

	// create a depth buffer
	createDepthResources ( vkDev, kScreenWidth, kScreenHeight, vkState.depthTexture );

	// initialize the descriptor pool, sets, passes, and the graphics pipeline
	const bool isInitialized =
		createDescriptorPool ( vkDev.device, static_cast<uint32_t>(vkDev.swapchainImages.size ()), 1, 2, 1, &vkState.descriptorPool ) &&
		createDescriptorSet () &&
		createColorAndDepthRenderPass ( vkDev, true, &vkState.renderPass, RenderPassCreateInfo{ .clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First | eRenderPassBit_Last } ) &&
		createPipelineLayout ( vkDev.device, vkState.descriptorSetLayout, &vkState.pipelineLayout ) &&
		createGraphicsPipeline ( vkDev.device, kScreenWidth, kScreenHeight, vkState.renderPass, vkState.pipelineLayout, shaderStages, &vkState.graphicsPipeline );

	if ( !isInitialized )
	{
		printf ( "Failed to create pipeline\n" );
		fflush ( stdout );
		exit ( EXIT_FAILURE );
	}

	createColorAndDepthFramebuffers ( vkDev, vkState.renderPass, vkState.depthTexture.imageView, vkState.swapchainFramebuffers );

	return VK_SUCCESS;
}

void terminateVulkan ()
{
	vkDestroyBuffer ( vkDev.device, vkState.storageBuffer, nullptr );
	vkFreeMemory ( vkDev.device, vkState.storageBufferMemory, nullptr );

	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		vkDestroyBuffer ( vkDev.device, vkState.uniformBuffers[i], nullptr );
		vkFreeMemory ( vkDev.device, vkState.uniformBuffersMemory[i], nullptr );
	}

	vkDestroyDescriptorSetLayout ( vkDev.device, vkState.descriptorSetLayout, nullptr );
	vkDestroyDescriptorPool ( vkDev.device, vkState.descriptorPool, nullptr );

	for ( auto framebuffer : vkState.swapchainFramebuffers )
	{
		vkDestroyFramebuffer ( vkDev.device, framebuffer, nullptr );
	}

	vkDestroySampler ( vkDev.device, vkState.textureSampler, nullptr );
	destroyVulkanTexture ( vkDev.device, vkState.texture );
	destroyVulkanTexture ( vkDev.device, vkState.depthTexture );
	
	vkDestroyRenderPass ( vkDev.device, vkState.renderPass, nullptr );

	vkDestroyPipelineLayout ( vkDev.device, vkState.pipelineLayout, nullptr );
	vkDestroyPipeline ( vkDev.device, vkState.graphicsPipeline, nullptr );

	destroyVulkanRenderDevice ( vkDev );

	destroyVulkanInstance ( vk );
}

bool drawOverlay ()
{
	// aquire the next available image from the swap chain and reset the command pool
	uint32_t imageIndex = 0;
	VK_CHECK ( vkAcquireNextImageKHR ( vkDev.device, vkDev.swapchain, 0, vkDev.semaphore, VK_NULL_HANDLE, &imageIndex ) );
	VK_CHECK ( vkResetCommandPool ( vkDev.device, vkDev.commandPool, 0 ) );

	// fill in the uniform buffer with data. rotate the model around the vertical axis
	int width, height;
	glfwGetFramebufferSize ( window, &width, &height );
	const float ratio = width / (float)height;
	const mat4 m1 = glm::rotate ( glm::translate ( mat4 ( 1.0f ), vec3 ( 0.0f, 0.5f, -1.5f ) ) * glm::rotate ( mat4 ( 1.0f ), glm::pi<float> (), vec3 ( 1, 0, 0 ) ), (float)glfwGetTime (), vec3 ( 0.0f, 1.0f, 0.0f ) );
	const mat4 p = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );
	const UniformBuffer ubo{ .mvp = p * m1 };

	updateUniformBuffer ( imageIndex, ubo );

	// fill the command buffers. not necessary in this recipe since the commands are identical
	fillCommandBuffers (imageIndex);

	const VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	const VkSubmitInfo si = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.semaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vkDev.commandBuffers[imageIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vkDev.renderSemaphore
	};

	VK_CHECK ( vkQueueSubmit ( vkDev.graphicsQueue, 1, &si, nullptr ) );

	const VkPresentInfoKHR pi = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vkDev.renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &vkDev.swapchain,
		.pImageIndices = &imageIndex
	};

	VK_CHECK ( vkQueuePresentKHR ( vkDev.graphicsQueue, &pi ) );
	VK_CHECK ( vkDeviceWaitIdle ( vkDev.device ) );
	return true;
}

int main ()
{
	// initialize the glslang compiler, the Volk library, and GLFW
	glslang_initialize_process ();
	volkInitialize ();
	if ( !glfwInit () )
	{
		exit ( EXIT_FAILURE );
	}
	if ( !glfwVulkanSupported () )
	{
		exit ( EXIT_FAILURE );
	}
	// disable OpenGL context creation
	glfwWindowHint ( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint ( GLFW_RESIZABLE, GL_FALSE );
	window = glfwCreateWindow ( kScreenWidth, kScreenHeight, "Vulkan Demo App", nullptr, nullptr );

	if ( !window )
	{
		glfwTerminate ();
		exit ( EXIT_FAILURE );
	}

	glfwSetKeyCallback (
		window,
		[]( GLFWwindow* window, int key, int scancode, int action, int mods )
		{
			if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
			{
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
			}
		}
	);

	initVulkan ();

	while ( !glfwWindowShouldClose ( window ) )
	{
		drawOverlay ();
		glfwPollEvents ();
	}

	terminateVulkan ();
	glfwTerminate ();
	glslang_finalize_process ();

	return 0;
}