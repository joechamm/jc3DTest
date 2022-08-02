#include <jcVkFramework/vkFramework/VulkanResources.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include <imgui/imgui.h>

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/load_ktx.hpp>

#include <algorithm>

VulkanResources::~VulkanResources ()
{
	for ( auto& t : allTextures_ )
	{
		destroyVulkanTexture ( vkDev_.device, t );
	}

	for ( auto& b : allBuffers_ )
	{
		vkDestroyBuffer ( vkDev_.device, b.buffer, nullptr );
		vkFreeMemory ( vkDev_.device, b.memory, nullptr );
	}

	for ( auto& fb : allFramebuffers_ )
	{
		vkDestroyFramebuffer ( vkDev_.device, fb, nullptr );
	}

	for ( auto& rp : allRenderpasses_ )
	{
		vkDestroyRenderPass ( vkDev_.device, rp, nullptr );
	}

	for ( auto& ds : allDSLayouts_ )
	{
		vkDestroyDescriptorSetLayout ( vkDev_.device, ds, nullptr );
	}

	for ( auto& pl : allPipelineLayouts_ )
	{
		vkDestroyPipelineLayout ( vkDev_.device, pl, nullptr );
	}

	for ( auto& p : allPipelines_ )
	{
		vkDestroyPipeline ( vkDev_.device, p, nullptr );
	}

	for ( auto& dpool : allDPools_ )
	{
		vkDestroyDescriptorPool ( vkDev_.device, dpool, nullptr );
	}
}

VulkanTexture VulkanResources::loadTexture2D ( const char* filename )
{
	VulkanTexture tex;
	if ( !createTextureImage ( vkDev_, filename, tex.image.image, tex.image.imageMemory ) )
	{
		printf ( "Cannot load %s 2D texture file\n", filename );
		exit ( EXIT_FAILURE );
	}

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	transitionImageLayout ( vkDev_, tex.image.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	if ( !createImageView ( vkDev_.device, tex.image.image, format, VK_IMAGE_ASPECT_COLOR_BIT, &tex.image.imageView ) )
	{
		printf ( "Cannot create image view for 2d texture (%s)\n", filename );
		exit ( EXIT_FAILURE );
	}

	createTextureSampler ( vkDev_.device, &tex.sampler );
	allTextures_.push_back ( tex );
	return tex;
}

VulkanTexture VulkanResources::loadCubeMap ( const char* filename, uint32_t mipLevels )
{
	VulkanTexture cubemap;

	uint32_t w = 0, h = 0;

	if ( mipLevels > 1 )
	{
		createMIPCubeTextureImage ( vkDev_, filename, mipLevels, cubemap.image.image, cubemap.image.imageMemory, &w, &h );
	} 
	else
	{
		createCubeTextureImage ( vkDev_, filename, cubemap.image.image, cubemap.image.imageMemory, &w, &h );
	}
	
	createImageView ( vkDev_.device, cubemap.image.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &cubemap.image.imageView, VK_IMAGE_VIEW_TYPE_CUBE, 6, mipLevels );

	createTextureSampler ( vkDev_.device, &cubemap.sampler );

	cubemap.format = VK_FORMAT_R32G32B32A32_SFLOAT;

	cubemap.width = w;
	cubemap.height = h;
	cubemap.depth = 1;

	allTextures_.push_back ( cubemap );
	return cubemap;
}

VulkanTexture VulkanResources::loadKTX ( const char* filename )
{
	gli::texture gliTex = gli::load_ktx ( filename );
	glm::tvec3<uint32_t> extent ( gliTex.extent ( 0 ) );

	VulkanTexture ktx = {
		.width = extent.x,
		.height = extent.y,
		.depth = 4
	};

	if ( !createTextureImageFromData ( vkDev_, ktx.image.image, ktx.image.imageMemory, (uint8_t*)gliTex.data ( 0, 0, 0 ), ktx.width, ktx.height, VK_FORMAT_R16G16_SFLOAT ) )
	{
		printf ( "ModelRenderer: failed to load BRDF LUT texture \n" );
		exit ( EXIT_FAILURE );
	}

	createImageView ( vkDev_.device, ktx.image.image, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &ktx.image.imageView );	
	createTextureSampler ( vkDev_.device, &ktx.sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );

	allTextures_.push_back ( ktx );

	return ktx;
}

VulkanTexture VulkanResources::createFontTexture ( const char* fontfile )
{
	ImGuiIO& io = ImGui::GetIO ();
	VulkanTexture res = { .image = {.image = VK_NULL_HANDLE } };

	// Build texture atlas
	ImFontConfig cfg = ImFontConfig ();
	cfg.FontDataOwnedByAtlas = false;
	cfg.RasterizerMultiply = 1.5f;
	cfg.SizePixels = 768.0f / 32.0f;
	cfg.PixelSnapH = true;
	cfg.OversampleH = 4;
	cfg.OversampleV = 4;
	ImFont* Font = io.Fonts->AddFontFromFileTTF ( fontfile, cfg.SizePixels, &cfg );

	unsigned char* pixels = nullptr;
	int texWidth = 1, texHeight = 1;
	io.Fonts->GetTexDataAsRGBA32 ( &pixels, &texWidth, &texHeight );

	if ( !pixels || !createTextureImageFromData ( vkDev_, res.image.image, res.image.imageMemory, pixels, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM ) )
	{
		printf ( "Failed to load texture\n" );
		return res;
	}

	createImageView ( vkDev_.device, res.image.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &res.image.imageView );
	createTextureSampler ( vkDev_.device, &res.sampler );

	/* This is not strictly necesary, a font can be any texture */
	io.Fonts->TexID = (ImTextureID)0;
	io.FontDefault = Font;
	io.DisplayFramebufferScale = ImVec2 ( 1, 1 );

	allTextures_.push_back ( res );
	return res;
}

VulkanTexture VulkanResources::addColorTexture ( int texWidth, int texHeight, VkFormat colorFormat, VkFilter minFilter, VkFilter maxFilter, VkSamplerAddressMode addressMode )
{
	const uint32_t w = (texWidth > 0) ? texWidth : vkDev_.framebufferWidth;
	const uint32_t h = (texHeight > 0) ? texHeight : vkDev_.framebufferHeight;

	VulkanTexture res = {
		.width = w,
		.height = h,
		.depth = 1,
		.format = colorFormat
	};

	if ( !createOffscreenImage ( vkDev_, res.image.image, res.image.imageMemory, w, h, colorFormat, 1, 0 ) )
	{
		printf ( "Cannot create color texture\n" );
		exit ( EXIT_FAILURE );
	}

	createImageView ( vkDev_.device, res.image.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, &res.image.imageView );
	createTextureSampler ( vkDev_.device, &res.sampler, minFilter, maxFilter, addressMode );

	transitionImageLayout ( vkDev_, res.image.image, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	allTextures_.push_back ( res );
	return res;
}

VulkanTexture VulkanResources::addDepthTexture ( int texWidth, int texHeight, VkImageLayout layout )
{
	const uint32_t w = (texWidth > 0) ? texWidth : vkDev_.framebufferWidth;
	const uint32_t h = (texHeight > 0) ? texHeight : vkDev_.framebufferHeight;

	const VkFormat depthFormat = findDepthFormat ( vkDev_.physicalDevice );

	VulkanTexture depth = {
		.width = w,
		.height = h,
		.depth = 1,
		.format = depthFormat
	};

	if ( !createImage ( vkDev_.device, vkDev_.physicalDevice, w, h, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth.image.image, depth.image.imageMemory ) )
	{
		printf ( "Cannot create depth texture\n" );
		exit ( EXIT_FAILURE );
	}

	createImageView ( vkDev_.device, depth.image.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &depth.image.imageView );
	transitionImageLayout ( vkDev_, depth.image.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, layout/*VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL*/ );

	if ( !createDepthSampler ( vkDev_.device, &depth.sampler ) )
	{
		printf ( "Cannot create a depth sampler\n" );
		exit ( EXIT_FAILURE );
	}

	allTextures_.push_back ( depth );
	return depth;
}

VulkanBuffer VulkanResources::addBuffer ( VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool createMapping )
{
	VulkanBuffer buffer = { .buffer = VK_NULL_HANDLE, .size = 0, .memory = VK_NULL_HANDLE, .ptr = nullptr };
	if ( !createSharedBuffer ( vkDev_, size, usage, properties, buffer.buffer, buffer.memory ) )
	{
		printf ( "Cannot allocate buffer\n" );
		exit ( EXIT_FAILURE );
	} else
	{
		buffer.size = size;
		allBuffers_.push_back ( buffer );
	}

	if ( createMapping )
	{
		vkMapMemory ( vkDev_.device, buffer.memory, 0, VK_WHOLE_SIZE, 0, &buffer.ptr );
	}

	return buffer;
}

VkFramebuffer VulkanResources::addFramebuffer ( RenderPass renderPass, const std::vector<VulkanTexture>& images )
{
	VkFramebuffer framebuffer;
	std::vector<VkImageView> attachments;
	for ( const auto& i : images )
	{
		attachments.push_back ( i.image.imageView );
	}

	VkFramebufferCreateInfo fbInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = renderPass.handle_,
		.attachmentCount = static_cast<uint32_t>(attachments.size ()),
		.pAttachments = attachments.data (),
		.width = images[0].width,
		.height = images[0].height,
		.layers = 1
	};

	if ( vkCreateFramebuffer ( vkDev_.device, &fbInfo, nullptr, &framebuffer ) != VK_SUCCESS )
	{
		printf ( "Unable to create offscreen framebuffer\n" );
		exit ( EXIT_FAILURE );
	}

	allFramebuffers_.push_back ( framebuffer );
	return framebuffer;
}

RenderPass VulkanResources::addRenderPass ( const std::vector<VulkanTexture>& outputs, const RenderPassCreateInfo& ci, bool useDepth )
{
	VkRenderPass renderPass;
	if ( outputs.empty () )
	{
		printf ( "Empty list of output attachments for RenderPass\n" );
		exit ( EXIT_FAILURE );
	}

	if ( outputs.size () == 1 )
	{
		if ( !createColorOnlyRenderPass ( vkDev_, &renderPass, ci, outputs[0].format ) )
		{
			printf ( "Unable to create offscreen color-only pass\n" );
			exit ( EXIT_FAILURE );
		}
	} else
	{
		if ( !createColorAndDepthRenderPass ( vkDev_, useDepth && (outputs.size () > 1), &renderPass, ci, outputs[0].format ) )
		{
			printf ( "Unable to create offscreen render pass\n" );
			exit ( EXIT_FAILURE );
		}
	}

	allRenderpasses_.push_back ( renderPass );
	RenderPass rp;
	rp.info_ = ci;
	rp.handle_ = renderPass;
	return rp;
}

RenderPass VulkanResources::addDepthRenderPass ( const std::vector<VulkanTexture>& outputs, const RenderPassCreateInfo& ci )
{
	VkRenderPass renderPass;
	if(!createDepthOnlyRender )

	return RenderPass ();
}
