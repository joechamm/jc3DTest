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

glslang_stage_t glslangShaderStageFromFileName ( const char* fileName );

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

VulkanTexture VulkanResources::addSolidRGBATexture ( uint32_t color )
{
	VulkanTexture tex;
	tex.width = 1;
	tex.height = 1;
	tex.depth = 1;
	tex.format = VK_FORMAT_R8G8B8A8_UNORM;
	if ( !createTextureImageFromData ( vkDev_, tex.image.image, tex.image.imageMemory, &color, 1, 1, tex.format ) )
	{
		printf ( "Cannot create solid texture\n" );
		exit ( EXIT_FAILURE );
	}

	transitionImageLayout ( vkDev_, tex.image.image, tex.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	
	if ( !createImageView ( vkDev_.device, tex.image.image, tex.format, VK_IMAGE_ASPECT_COLOR_BIT, &tex.image.imageView ) )
	{
		printf ( "Cannot create image view for solid texture\n" );
		exit ( EXIT_FAILURE );
	}

	createTextureSampler ( vkDev_.device, &tex.sampler );
	allTextures_.push_back ( tex );

	return tex;	
}

VulkanTexture VulkanResources::addRGBATexture ( int texWidth, int texHeight, void* data )
{
	VulkanTexture tex;
	tex.width = texWidth;
	tex.height = texHeight;
	tex.depth = 1;
	tex.format = VK_FORMAT_R8G8B8A8_UNORM;
	if ( !createTextureImageFromData ( vkDev_, tex.image.image, tex.image.imageMemory, data, texWidth, texHeight, tex.format ) )
	{
		printf ( "Cannot create RGBA texture\n" );
		exit ( EXIT_FAILURE );
	}

	transitionImageLayout ( vkDev_, tex.image.image, tex.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

	if ( !createImageView ( vkDev_.device, tex.image.image, tex.format, VK_IMAGE_ASPECT_COLOR_BIT, &tex.image.imageView ) )
	{
		printf ( "Cannot create image view for RGBA texture\n" );
		exit ( EXIT_FAILURE );
	}

	createTextureSampler ( vkDev_.device, &tex.sampler );
	allTextures_.push_back ( tex );

	return tex;
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



VulkanBuffer VulkanResources::addVertexBuffer ( uint32_t indexBufferSize, const void* indexData, uint32_t vertexBufferSize, const void* vertexData )
{
	VulkanBuffer result;
	result.size = allocateVertexBuffer ( vkDev_, &result.buffer, &result.memory, vertexBufferSize, vertexData, indexBufferSize, indexData );
	allBuffers_.push_back ( result );

	return result;
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

RenderPass VulkanResources::addFullScreenPass ( bool useDepth, const RenderPassCreateInfo& ci )
{
	RenderPass result ( vkDev_, useDepth, ci );
	allRenderpasses_.push_back ( result.handle_ );
	return result;
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
	if(!createDepthOnlyRenderPass(vkDev_, &renderPass, ci))
	{
		printf("Unable to create depth-only render pass\n");
		exit(EXIT_FAILURE);
	}

	allRenderpasses_.push_back ( renderPass );
	RenderPass rp;
	rp.info_ = ci;
	rp.handle_ = renderPass;
	return rp;
}

VkPipelineLayout VulkanResources::addPipelineLayout ( VkDescriptorSetLayout dsLayout, uint32_t vtxConstSize, uint32_t fragConstSize )
{
	VkPipelineLayout pipelineLayout;
	if ( !createPipelineLayoutWithConstants ( vkDev_.device, dsLayout, &pipelineLayout, vtxConstSize, fragConstSize ) )
	{
		printf ( "Cannot create pipeline layout\n" );
		exit ( EXIT_FAILURE );
	}

	allPipelineLayouts_.push_back ( pipelineLayout );
	return pipelineLayout;
}

VkPipeline VulkanResources::addPipeline ( VkRenderPass renderPass, VkPipelineLayout layout, const std::vector<std::string>& shaderNames, const PipelineInfo& pi )
{
	VkPipeline pipeline;
	if ( !createGraphicsPipeline ( vkDev_, renderPass, layout, shaderNames, &pipeline, pi.topology_, pi.useDepth_, pi.useBlending_, pi.dynamicScissorState_, pi.width_, pi.height_, pi.patchControlPoints_ ) )
	{
		printf ( "Cannot create graphics pipeline\n" );
		exit ( EXIT_FAILURE );
	}

	allPipelines_.push_back ( pipeline );
	return pipeline;
}

VkDescriptorPool VulkanResources::addDescriptorPool ( const DescriptorSetInfo& dsInfo, uint32_t dSetCount )
{
	uint32_t uniformBufferCount = 0;
	uint32_t storageBufferCount = 0;
	uint32_t samplerCount = static_cast<uint32_t>(dsInfo.textures_.size ());

	for ( const auto& ta : dsInfo.textureArrays_ )
	{
		samplerCount += static_cast<uint32_t>(ta.textureArray_.size ());
	}

	for ( const auto& b : dsInfo.buffers_ )
	{
		if ( b.dInfo_.type_ == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER )
		{
			uniformBufferCount++;
		}
		if ( b.dInfo_.type_ == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER )
		{
			storageBufferCount++;
		}
	}

	std::vector<VkDescriptorPoolSize> poolSizes;

	/* printf("Allocating pool[%d | %d | %d]\n", (int)uniformBufferCount, (int)storageBufferCount, (int)samplerCount); */
	
	if ( uniformBufferCount )
	{
		poolSizes.push_back ( VkDescriptorPoolSize {
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = dSetCount * uniformBufferCount
		} );
	}

	if ( storageBufferCount )
	{
		poolSizes.push_back ( VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = dSetCount * storageBufferCount
		} );
	}

	if ( samplerCount )
	{
		poolSizes.push_back ( VkDescriptorPoolSize{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = dSetCount * samplerCount
		} );
	}
	
	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = static_cast<uint32_t>(dSetCount),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size ()),
		.pPoolSizes = poolSizes.empty () ? nullptr : poolSizes.data ()
	};

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

	if( vkCreateDescriptorPool ( vkDev_.device, &poolInfo, nullptr, &descriptorPool ) != VK_SUCCESS )
	{
		printf ( "Cannot create descriptor pool\n" );
		exit ( EXIT_FAILURE );
	}

	allDPools_.push_back ( descriptorPool );
	return descriptorPool;
}

VkDescriptorSetLayout VulkanResources::addDescriptorSetLayout ( const DescriptorSetInfo& dsInfo )
{
	VkDescriptorSetLayout descriptorSetLayout;

	uint32_t bindingIdx = 0;

	std::vector<VkDescriptorSetLayoutBinding> bindings;

	for ( const auto& b : dsInfo.buffers_ )
	{
		bindings.push_back ( descriptorSetLayoutBinding ( bindingIdx++, b.dInfo_.type_, b.dInfo_.shaderStageFlags_ ) );
	}

	for ( const auto& i : dsInfo.textures_ )
	{
		bindings.push_back ( descriptorSetLayoutBinding ( bindingIdx++, i.dInfo_.type_ /*VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER*/, i.dInfo_.shaderStageFlags_ ) );
	}

	for ( const auto& t : dsInfo.textureArrays_ )
	{
		bindings.push_back ( descriptorSetLayoutBinding ( bindingIdx++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, t.dInfo_.shaderStageFlags_, static_cast<uint32_t>(t.textureArray_.size ()) ) );
	}

	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size ()),
		.pBindings = bindings.size () > 0 ? bindings.data () : nullptr
	};

	if ( vkCreateDescriptorSetLayout ( vkDev_.device, &layoutInfo, nullptr, &descriptorSetLayout ) != VK_SUCCESS )
	{
		printf ( "Cannot create descriptor set layout\n" );
		exit ( EXIT_FAILURE );
	}
	
	allDSLayouts_.push_back ( descriptorSetLayout );
	return descriptorSetLayout;	
}

VkDescriptorSet VulkanResources::addDescriptorSet ( VkDescriptorPool descriptorPool, VkDescriptorSetLayout dsLayout )
{
	VkDescriptorSet descriptorSet;

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &dsLayout
	};

	if ( vkAllocateDescriptorSets ( vkDev_.device, &allocInfo, &descriptorSet ) != VK_SUCCESS )
	{
		printf ( "Cannot allocate descriptor set\n" );
		exit ( EXIT_FAILURE );
	}

	return descriptorSet;
}
/* This routine counts all textures in all texture arrays (if any of them are present), creates a list of DescriptorWrite opertations, with required buffer/image info structures and calls the vkUpdateDescriptorSets() */
void VulkanResources::updateDescriptorSet ( VkDescriptorSet ds, const DescriptorSetInfo& dsInfo )
{
	uint32_t bindingIdx = 0;
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	std::vector<VkDescriptorBufferInfo> bufferDescriptors ( dsInfo.buffers_.size () );
	std::vector<VkDescriptorImageInfo> imageDescriptors ( dsInfo.textures_.size () );
	std::vector<VkDescriptorImageInfo> imageArrayDescriptors;

	for ( size_t i = 0; i < dsInfo.buffers_.size (); i++ )
	{
		BufferAttachment b = dsInfo.buffers_[i];

		bufferDescriptors[i] = VkDescriptorBufferInfo{
			.buffer = b.buffer_.buffer,
			.offset = b.offset_,
			.range = (b.size_ > 0) ? b.size_ : VK_WHOLE_SIZE
		};

		descriptorWrites.push_back ( bufferWriteDescriptorSet ( ds, &bufferDescriptors[i], bindingIdx++, b.dInfo_.type_ ) );
	}

	for ( size_t i = 0; i < dsInfo.textures_.size (); i++ )
	{
		VulkanTexture t = dsInfo.textures_[i].texture_;

		imageDescriptors[i] = VkDescriptorImageInfo{
			.sampler = t.sampler,
			.imageView = t.image.imageView,
			.imageLayout = /* t.texture.layout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		descriptorWrites.push_back ( imageWriteDescriptorSet ( ds, &imageDescriptors[i], bindingIdx++ ) );
	}

	uint32_t taOffset = 0;
	std::vector<uint32_t> taOffsets ( dsInfo.textureArrays_.size () );
	for ( size_t ta = 0; ta < dsInfo.textureArrays_.size (); ta++ )
	{
		taOffsets[ta] = taOffset;

		for ( size_t j = 0; j < dsInfo.textureArrays_[ta].textureArray_.size (); j++ )
		{
			VulkanTexture t = dsInfo.textureArrays_[ta].textureArray_[j];

			VkDescriptorImageInfo imageInfo = {
				.sampler = t.sampler,
				.imageView = t.image.imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			imageArrayDescriptors.push_back ( imageInfo ); // item 'taOffsets[ta] + j'
		}

		taOffset += static_cast<uint32_t>(dsInfo.textureArrays_[ta].textureArray_.size ());
	}

	for ( size_t ta = 0; ta < dsInfo.textureArrays_.size (); ta++ )
	{
		VkWriteDescriptorSet writeSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = ds,
			.dstBinding = bindingIdx++,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(dsInfo.textureArrays_[ta].textureArray_.size ()),
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = imageArrayDescriptors.data () + taOffsets[ta]
		};

		descriptorWrites.push_back ( writeSet );
	}

	vkUpdateDescriptorSets ( vkDev_.device, static_cast<uint32_t>(descriptorWrites.size ()), descriptorWrites.data (), 0, nullptr );
}

/* Helper mesh-related functions */
std::pair<BufferAttachment, BufferAttachment> VulkanResources::makeMeshBuffers ( const std::vector<float>& vertices, const std::vector<unsigned int>& indices )
{
	const uint32_t indexBufferSize = static_cast<uint32_t> ( indices.size () * sizeof ( int ) );
	const uint32_t vertexBufferSize = static_cast<uint32_t> ( vertices.size () * sizeof ( float ) );

	VulkanBuffer storageBuffer = addVertexBuffer ( indexBufferSize, indices.data (), vertexBufferSize, vertices.data () );

	BufferAttachment vertexBufferAttachment { .dInfo_ = {.type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .shaderStageFlags_ = VK_SHADER_STAGE_VERTEX_BIT }, .buffer_ = storageBuffer, .offset_ = 0, .size_ = vertexBufferSize };
	BufferAttachment indexBufferAttachment { .dInfo_ = { .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .shaderStageFlags_ = VK_SHADER_STAGE_VERTEX_BIT }, .buffer_ = storageBuffer, .offset_ = vertexBufferSize, .size_ = indexBufferSize };

	return { vertexBufferAttachment, indexBufferAttachment };
}

std::pair<BufferAttachment, BufferAttachment> VulkanResources::loadMeshToBuffer ( const std::string& meshFilename, bool useTextureCoordinates, bool useNormals, std::vector<float>& vertices, std::vector<unsigned int>& indices )
{
	const aiScene* scene = aiImportFile ( meshFilename.c_str (), aiProcess_Triangulate );

	if ( !scene || !scene->HasMeshes () )
	{
		printf("Error: Could not load mesh file %s\n",meshFilename.c_str());
		VulkanBuffer nullBuffer{ VK_NULL_HANDLE, 0, VK_NULL_HANDLE };

		return std::pair{ BufferAttachment{DescriptorInfo{}, nullBuffer}, BufferAttachment{DescriptorInfo{}, nullBuffer} };
	}

	const aiMesh* mesh = scene->mMeshes[0];			

	for ( unsigned i = 0; i != mesh->mNumVertices; i++ )
	{
		const aiVector3D v = mesh->mVertices[i];
		const aiVector3D t = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][i] : aiVector3D ();
		const aiVector3D n = mesh->mNormals ? mesh->mNormals[i] : aiVector3D ();

		vertices.push_back ( v.x );
		vertices.push_back ( v.y );
		vertices.push_back ( v.z );
		
		if ( useTextureCoordinates )
		{
			vertices.push_back ( t.x );
			vertices.push_back ( t.y );
		}

		if ( useNormals )
		{
			vertices.push_back ( n.x );
			vertices.push_back ( n.y );
			vertices.push_back ( n.z );
		}
	}

	for ( unsigned i = 0; i != mesh->mNumFaces; i++ )
	{
		for ( unsigned j = 0; j != 3; j++ )
		{
			indices.push_back ( mesh->mFaces[i].mIndices[j] );
		}
	}

	aiReleaseImport ( scene );

	const uint32_t vertexBufferSize = static_cast<uint32_t>(sizeof(float) * vertices.size ());
	const uint32_t indexBufferSize = static_cast<uint32_t>(sizeof ( unsigned int ) * indices.size ());

	return makeMeshBuffers ( vertices, indices );	
}

std::pair<BufferAttachment, BufferAttachment> VulkanResources::createPlaneBuffer_XZ ( float sx, float sz )
{
	return makeMeshBuffers (
		std::vector<float> {
		-sx, 0, -sz, 0, 0, 0, 1, 0,
			+sx, 0, -sz, 1, 0, 0, 1, 0,
			+sx, 0, +sz, 1, 1, 0, 1, 0,
			-sx, 0, +sz, 0, 1, 0, 1, 0
	},
		std::vector<unsigned int> { 0u, 1u, 2u, 0u, 3u, 2u } );
}

std::pair<BufferAttachment, BufferAttachment> VulkanResources::createPlaneBuffer_XY ( float sx, float sy )
{
	return makeMeshBuffers (
		std::vector<float> {
		-sx, -sy, 0, 0, 0, 0, 0, 1,
			+sx, -sy, 0, 1, 0, 0, 0, 1,
			+sx, +sy, 0, 1, 1, 0, 0, 1,
			-sx, +sy, 0, 0, 1, 0, 0, 1
	},
		std::vector<unsigned int> { 0u, 1u, 2u, 0u, 3u, 2u } );
}

std::vector<VkFramebuffer> VulkanResources::addFramebuffers ( VkRenderPass renderPass, VkImageView depthView )
{
	std::vector<VkFramebuffer> framebuffers;
	createColorAndDepthFramebuffers ( vkDev_, renderPass, depthView, framebuffers );
	for ( auto f : framebuffers )
	{
		allFramebuffers_.push_back ( f );
	}

	return framebuffers;
}

bool VulkanResources::createGraphicsPipeline ( VulkanRenderDevice& vkdev, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, const std::vector<std::string>& shaderFiles, VkPipeline* pipeline, VkPrimitiveTopology topology, bool useDepth, bool useBlending, bool dynamicScissorState, int32_t customWidth, int32_t customHeight, uint32_t numPatchControlPoints )
{
	std::vector<ShaderModule> localShaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderStages.resize ( shaderFiles.size () );
	localShaderModules.resize ( shaderFiles.size () );

	for ( size_t i = 0; i < shaderFiles.size (); i++ )
	{
		const std::string& shaderFile = shaderFiles[i];

		auto idx = shaderMap_.find ( shaderFile );

		if ( idx != shaderMap_.end () )
		{
			// printf("Already compiled file (%s)\n", shaderFile.c_str());
			localShaderModules[i] = shaderModules_[idx->second];
		} else
		{
			// printf("Compiling file (%s)\n", shaderFiles[i].c_str());
			VK_CHECK(createShaderModule(vkDev_.device, &localShaderModules[i], shaderFile.c_str()));
			shaderModules_.push_back ( localShaderModules[i] );
			shaderMap_[shaderFile] = (int)shaderModules_.size () - 1;
		}

		VkShaderStageFlagBits stage = glslangShaderStageToVulkan ( glslangShaderStageFromFileName ( shaderFile.c_str () ) );

		shaderStages[i] = shaderStageInfo ( stage, localShaderModules[i], "main" );	
		
	}

	const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		/* The only difference from createGraphicsPipeline() */
		.topology = topology,
		.primitiveRestartEnable = VK_FALSE
	};

	const VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(customWidth > 0 ? customWidth : vkDev_.framebufferWidth),
		.height = static_cast<float>(customHeight > 0 ? customHeight : vkDev_.framebufferHeight),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	const VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = { customWidth > 0 ? customWidth : vkDev_.framebufferWidth, customHeight > 0 ? customHeight : vkDev_.framebufferHeight }
	};

	const VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	const VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f
	};

	const VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f		
	};

	const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT	
	};

	const VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	const VkPipelineDepthStencilStateCreateInfo depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
		.depthWriteEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f	
	};
	
	VkDynamicState dynamicStateElt = VK_DYNAMIC_STATE_SCISSOR;

	const VkPipelineDynamicStateCreateInfo dynamicState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.dynamicStateCount = 1,
		.pDynamicStates = &dynamicStateElt
	};

	const VkPipelineTessellationStateCreateInfo tessellationState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.patchControlPoints = numPatchControlPoints
	};	
	
	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	
		.stageCount = static_cast<uint32_t>(shaderStages.size ()),
		.pStages = shaderStages.data (),
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = (topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &tessellationState : nullptr,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = useDepth ? &depthStencil : nullptr,
		.pColorBlendState = &colorBlending,
		.pDynamicState = dynamicScissorState ? &dynamicState : nullptr,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	VK_CHECK ( vkCreateGraphicsPipelines ( vkDev_.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline ) );

	return true;
}
