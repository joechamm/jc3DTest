#include <imgui/imgui.h>

#include "vkRenderers/VulkanImGui.h"

#include "ResourceString.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

using std::string;

constexpr uint32_t ImGuiVtxBufferSize = 512 * 1024 * sizeof ( ImDrawVert );
constexpr uint32_t ImGuiIdxBufferSize = 512 * 1024 * sizeof ( uint32_t );

bool createFontTexture ( ImGuiIO& io, const char* fontFile, VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory )
{
	// Build texture atlas
	ImFontConfig cfg = ImFontConfig ();
	cfg.FontDataOwnedByAtlas = false;
	cfg.RasterizerMultiply = 1.5f;
	cfg.SizePixels = 768.0f / 32.0f;
	cfg.PixelSnapH = true;
	cfg.OversampleH = 4;
	cfg.OversampleV = 4;
	ImFont* Font = io.Fonts->AddFontFromFileTTF ( fontFile, cfg.SizePixels, &cfg );

	unsigned char* pixels = nullptr;
	int texWidth = 1, texHeight = 1;
	io.Fonts->GetTexDataAsRGBA32 ( &pixels, &texWidth, &texHeight );

	if ( !pixels || !createTextureImageFromData ( vkDev, textureImage, textureImageMemory, pixels, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM ) )
	{
		printf ( "ImGuiRenderer: Failed to load texture\n" );
		fflush ( stdout );
		return false;
	}

	io.Fonts->TexID = (ImTextureID)0;
	io.FontDefault = Font;
	io.DisplayFramebufferScale = ImVec2 ( 1, 1 );

	return true;
}

void addImGuiItem ( uint32_t width, uint32_t height, VkCommandBuffer commandBuffer, const ImDrawCmd* pcmd, ImVec2 clipOff, ImVec2 clipScale, int idxOffset, int vtxOffset )
{
	if ( pcmd->UserCallback ) return;
	ImVec4 clipRect;
	clipRect.x = (pcmd->ClipRect.x - clipOff.x) * clipScale.x;
	clipRect.y = (pcmd->ClipRect.y - clipOff.y) * clipScale.y;
	clipRect.z = (pcmd->ClipRect.z - clipOff.x) * clipScale.x;
	clipRect.w = (pcmd->ClipRect.w - clipOff.y) * clipScale.y;
	if ( clipRect.x < width && clipRect.y < height && clipRect.z >= 0.0f && clipRect.w >= 0.0f )
	{
		if ( clipRect.x < 0.0f ) clipRect.x = 0.0f;
		if ( clipRect.y < 0.0f ) clipRect.y = 0.0f;

		// Apply scissor/clipping rectangle
		const VkRect2D scissor = {
			.offset = {.x = (int32_t)(clipRect.x), .y = (int32_t)(clipRect.y) },
			.extent = {.width = (uint32_t)(clipRect.z - clipRect.x), .height = (uint32_t)(clipRect.w - clipRect.y) }
		};

		vkCmdSetScissor ( commandBuffer, 0, 1, &scissor );

		vkCmdDraw ( commandBuffer, pcmd->ElemCount, 1, pcmd->IdxOffset + idxOffset, pcmd->VtxOffset + vtxOffset );
	}
}

ImGuiRenderer::ImGuiRenderer ( VulkanRenderDevice& vkDev ) : RendererBase ( vkDev, VulkanImage () )
{
	ImGuiIO& io = ImGui::GetIO ();

	string fontFilename = appendToRoot ( "assets/fonts/OpenSans-Light.ttf" );
	createFontTexture ( io, fontFilename.c_str (), vkDev, font_.image, font_.imageMemory );

	createImageView ( vkDev.device, font_.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &font_.imageView );
	createTextureSampler ( vkDev.device, &fontSampler_ );

	// Buffer allocation
	const size_t imgCount = vkDev.swapchainImages.size ();

	storageBuffer_.resize ( imgCount );
	storageBufferMemory_.resize ( imgCount );

	bufferSize_ = ImGuiVtxBufferSize + ImGuiIdxBufferSize;

	for ( size_t i = 0; i < imgCount; i++ )
	{
		if ( !createBuffer ( vkDev.device, vkDev.physicalDevice, bufferSize_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, storageBuffer_[i], storageBufferMemory_[i] ) )
		{
			printf ( "ImGuiRenderer: createBuffer() failed\n" );
			exit ( EXIT_FAILURE );
		}
	}

	// Pipeline creation

	if ( !createColorAndDepthRenderPass ( vkDev, false, &renderPass_, RenderPassCreateInfo () ) )
	{
		printf ( "ImGuiRenderer: failed to create color and depth render pass\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createColorAndDepthFramebuffers ( vkDev, renderPass_, VK_NULL_HANDLE, swapchainFramebuffers_ ) )
	{
		printf ( "ImGuiRenderer: failed to create color framebuffers\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createUniformBuffers ( vkDev, sizeof ( mat4 ) ) )
	{
		printf ( "ImGuiRenderer: failed to create uniform buffers\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createDescriptorPool ( vkDev, 1, 2, 1, &descriptorPool_ ) )
	{
		printf ( "ImGuiRenderer: failed to create descriptor pool\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createDescriptorSet ( vkDev ) )
	{
		printf ( "ImGuiRenderer: failed to create descriptor set\n" );
		exit ( EXIT_FAILURE );
	}

	if ( !createPipelineLayout ( vkDev.device, descriptorSetLayout_, &pipelineLayout_ ) )
	{
		printf ( "ImGuiRenderer: failed to create pipeline layout\n" );
		exit ( EXIT_FAILURE );
	}

	std::vector<string> shaderFilenames = getShaderFilenamesWithRoot ( "assets/shaders/imgui.vert", "assets/shaders/imgui.frag" );

	if ( !createGraphicsPipeline ( vkDev, renderPass_, pipelineLayout_, shaderFilenames, &graphicsPipeline_, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true, true, true ) )
	{
		printf ( "ImGuiRenderer: failed to create graphics pipeline\n" );
		exit ( EXIT_FAILURE );
	}
}

ImGuiRenderer::~ImGuiRenderer ()
{
	for ( size_t i = 0; i < swapchainFramebuffers_.size (); i++ )
	{
		vkDestroyBuffer ( device_, storageBuffer_[i], nullptr );
		vkFreeMemory ( device_, storageBufferMemory_[i], nullptr );
	}

	vkDestroySampler ( device_, fontSampler_, nullptr );
	destroyVulkanImage ( device_, font_ );
}


bool ImGuiRenderer::createDescriptorSet ( VulkanRenderDevice& vkDev )
{
	const std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
		descriptorSetLayoutBinding ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		descriptorSetLayoutBinding ( 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT )
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size ()),
		.pBindings = bindings.data ()
	};

	VK_CHECK ( vkCreateDescriptorSetLayout ( vkDev.device, &layoutInfo, nullptr, &descriptorSetLayout_ ) );

	std::vector<VkDescriptorSetLayout> layouts ( vkDev.swapchainImages.size (), descriptorSetLayout_ );

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(vkDev.swapchainImages.size ()),
		.pSetLayouts = layouts.data ()
	};

	descriptorSets_.resize ( vkDev.swapchainImages.size () );

	VK_CHECK ( vkAllocateDescriptorSets ( vkDev.device, &allocInfo, descriptorSets_.data () ) );

	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		VkDescriptorSet ds = descriptorSets_[i];

		const VkDescriptorBufferInfo bufferInfo = {
			uniformBuffers_[i], 0, sizeof(mat4)
		};

		const VkDescriptorBufferInfo bufferInfo2 = {
			storageBuffer_[i], 0, ImGuiVtxBufferSize
		};

		const VkDescriptorBufferInfo bufferInfo3 = {
			storageBuffer_[i], ImGuiVtxBufferSize, ImGuiIdxBufferSize
		};

		const VkDescriptorImageInfo imageInfo = {
			fontSampler_,
			font_.imageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		const std::array<VkWriteDescriptorSet, 4> descriptorWrites = {
			bufferWriteDescriptorSet ( ds, &bufferInfo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ),
			bufferWriteDescriptorSet ( ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ),
			bufferWriteDescriptorSet ( ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ),
			imageWriteDescriptorSet ( ds, &imageInfo, 3 )
		};

		vkUpdateDescriptorSets ( vkDev.device, static_cast<uint32_t>(descriptorWrites.size ()), descriptorWrites.data (), 0, nullptr );
	}

	return true;
}

void ImGuiRenderer::fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage )
{
	beginRenderPass ( commandBuffer, currentImage );

	ImVec2 clipOff = drawData->DisplayPos;  
	ImVec2 clipScale = drawData->FramebufferScale;

	int vtxOffset = 0;
	int idxOffset = 0;

	for ( int n = 0; n < drawData->CmdListsCount; n++ )
	{
		const ImDrawList* cmdList = drawData->CmdLists[n];
		for ( int cmd = 0; cmd < cmdList->CmdBuffer.Size; cmd++ )
		{
			const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd];
			addImGuiItem ( framebufferWidth_, framebufferHeight_, commandBuffer, pcmd, clipOff, clipScale, idxOffset, vtxOffset );
			idxOffset += cmdList->IdxBuffer.Size;
			vtxOffset += cmdList->VtxBuffer.Size;
		}
	}

	vkCmdEndRenderPass ( commandBuffer );
}

void ImGuiRenderer::updateBuffers ( VulkanRenderDevice& vkDev, uint32_t currentImage, const ImDrawData* imguiDrawData )
{
	drawData = imguiDrawData;
	const float L = drawData->DisplayPos.x;
	const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
	const float T = drawData->DisplayPos.y;
	const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
	const mat4 inMtx = glm::ortho ( L, R, T, B );
	uploadBufferData ( vkDev, uniformBuffersMemory_[currentImage], 0, glm::value_ptr ( inMtx ), sizeof ( mat4 ) );
	void* data = nullptr;
	vkMapMemory ( vkDev.device, storageBufferMemory_[currentImage], 0, bufferSize_, 0, &data );
	ImDrawVert* vtx = (ImDrawVert*)data;
	for ( int n = 0; n < drawData->CmdListsCount; n++ )
	{
		const ImDrawList* cmdList = drawData->CmdLists[n];
		memcpy ( vtx, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof ( ImDrawVert ) );
		vtx += cmdList->VtxBuffer.Size;
	}

	uint32_t* idx = (uint32_t*)((uint8_t*)data + ImGuiVtxBufferSize);
	for ( int n = 0; n < drawData->CmdListsCount; n++ )
	{
		const ImDrawList* cmdList = drawData->CmdLists[n];
		const uint16_t* src = (const uint16_t*)cmdList->IdxBuffer.Data;
		for ( int j = 0; j < cmdList->IdxBuffer.Size; j++ )
		{
			*idx++ = (uint32_t)*src++;
		}
	}

	vkUnmapMemory ( vkDev.device, storageBufferMemory_[currentImage] );
}