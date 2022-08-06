#include <jcVkFramework/vkFramework/MultiRenderer.h>
#include <jcCommonFramework/ResourceString.h>
#include <stb/stb_image.h>

// helper function for setting object names
static std::string getBasePath ( const std::string& fname )
{
	const std::size_t pathSeparator = fname.find_last_of ( "/\\" );
	return (pathSeparator != std::string::npos) ? fname.substr ( 0, pathSeparator + 1 ) : std::string ();
}

static std::string getShortName ( const std::string& fname )
{
	const std::size_t pathSeparator = fname.find_last_of ( "/\\" );
	return (pathSeparator != std::string::npos) ? fname.substr ( pathSeparator + 1 ) : fname;
}

/// Draw a checkerboard on a pre-allocated square RGB image.
uint8_t* genDefaultCheckerboardImage ( int* width, int* height )
{
	const int w = 128;
	const int h = 128;

	uint8_t* imgData = (uint8_t*)malloc ( w * h * 3 ); // stbi_load() uses malloc, so this is safe

	assert ( imgData && w > 0 && h > 0 );
	assert ( w == h );
	
	if ( !imgData || w <= 0 || h <= 0 ) return nullptr;
	if(w != h) return nullptr;

	for ( int i = 0; i < w * h; i++ )
	{
		const int row = i / w;
		const int col = i % w;
		imgData[i * 3 + 0] = imgData[i * 3 + 1] = imgData[i * 3 + 2] = 0xFF * ((row + col) % 2);
	}

	if ( width ) *width = w;
	if ( height ) *height = h;

	return imgData;
}

VKSceneData::VKSceneData ( VulkanRenderContext& ctx, 
	const std::string& meshFile, 
	const std::string& sceneFile, 
	const std::string& materialFile, 
	VulkanTexture envMap, 
	VulkanTexture irradianceMap, 
	bool asyncLoad )
	: ctx_(ctx)
	, envMapIrradiance_(irradianceMap)
	, envMap_(envMap)
{
	std::string sceneName = getShortName ( sceneFile );
	std::string meshName = getShortName ( meshFile );
	std::string materialName = getShortName ( materialFile );

	brdfLUT_ = ctx.resources_.loadKTX ( appendToRoot ( "assets/images/brdfLUT.ktx" ).c_str () );

	setVkObjectName ( ctx.vkDev_.device, (uint64_t)brdfLUT_.image.image, VK_OBJECT_TYPE_IMAGE, "brdfLUT" );
	setVkObjectName ( ctx.vkDev_.device, (uint64_t)envMap_.image.image, VK_OBJECT_TYPE_IMAGE, "envMap" );
	setVkObjectName ( ctx.vkDev_.device, (uint64_t)envMapIrradiance_.image.image, VK_OBJECT_TYPE_IMAGE, "envIrrMap" );

	loadMaterials ( materialFile.c_str (), materials_, textureFiles_ );

	std::vector<VulkanTexture> textures;
	for ( const auto& f : textureFiles_ )
	{
		auto t = asyncLoad ? ctx.resources_.addSolidRGBATexture () : ctx.resources_.loadTexture2D ( f.c_str () );
		textures.push_back ( t );

		setVkObjectName ( ctx.vkDev_.device, (uint64_t)t.image.image, VK_OBJECT_TYPE_IMAGE, f.c_str () );
#if 0
		if ( t.image.image != nullptr )
		{
			setVkImageName(ctx.vkDev_, t.image.image, f.c_str());
		}
#endif
		
	}

	if ( asyncLoad )
	{
		loadedFiles_.reserve ( textureFiles_.size () );

		taskflow_.for_each_index ( 0u, (uint32_t)textureFiles_.size (), 1u, [this]( int idx )
			{
				int w, h;
				const uint8_t* img = stbi_load ( this->textureFiles_[idx].c_str (), &w, &h, nullptr, STBI_rgb_alpha );
				if ( !img )
				{
					img = genDefaultCheckerboardImage ( &w, &h );
				}
				std::lock_guard lock ( loadedFilesMutex_ );
				loadedFiles_.emplace_back ( LoadedImageData{ idx, w, h, img } );
			}
		);

		executor_.run ( taskflow_ );
	}

	allMaterialTextures_ = fsTextureArrayAttachment ( textures );

	const uint32_t materialSize = static_cast<uint32_t>(sizeof ( MaterialDescription ) * materials_.size ());
	material_ = ctx.resources_.addStorageBuffer ( materialSize );

	setVkObjectName ( ctx.vkDev_.device, (uint64_t)material_.buffer, VK_OBJECT_TYPE_BUFFER, materialName.c_str () );

	uploadBufferData ( ctx.vkDev_, material_.memory, 0, materials_.data (), materialSize );

	loadMeshes ( meshFile );

	std::string buffName = meshName + ":storage";
	setVkObjectName ( ctx.vkDev_.device, (uint64_t)vertexBuffer_.buffer_.buffer, VK_OBJECT_TYPE_BUFFER, buffName.c_str () );	

	loadScene ( sceneFile );	

	std::string transformsName = sceneName + ":transforms";
	setVkObjectName ( ctx.vkDev_.device, (uint64_t)transforms_.buffer, VK_OBJECT_TYPE_BUFFER, transformsName.c_str () );
}

void VKSceneData::loadScene ( const std::string& sceneFile )
{
	::loadScene ( sceneFile.c_str (), scene_ );

	// prepare draw data buffer
	for ( const auto& c : scene_.meshes_ )
	{
		auto material = scene_.materialForNode_.find ( c.first );
		if ( material == scene_.materialForNode_.end () ) continue;
		
		shapes_.push_back (
			DrawData{
				.meshIndex = c.second,
				.materialIndex = material->second,
				.LOD = 0,
				.indexOffset = meshData_.meshes_[c.second].indexOffset,
				.vertexOffset = meshData_.meshes_[c.second].vertexOffset,
				.transformIndex = c.first
			}
		);
	}

	shapeTransforms_.resize ( shapes_.size () );
	transforms_ = ctx_.resources_.addStorageBuffer ( shapes_.size () * sizeof ( glm::mat4 ) );

	recalculateAllTransforms ();
	uploadGlobalTransforms ();
}

void VKSceneData::loadMeshes ( const std::string & meshFile )
{
	MeshFileHeader header = loadMeshData ( meshFile.c_str (), meshData_ );

	const uint32_t indexBufferSize = header.indexDataSize;
	uint32_t vertexBufferSize = header.vertexDataSize;

	const uint32_t offsetAlignment = getVulkanBufferAlignment ( ctx_.vkDev_ );
	if ( (vertexBufferSize & (offsetAlignment - 1)) != 0 )
	{
		const size_t numFloats = (offsetAlignment - (vertexBufferSize & (offsetAlignment - 1))) / sizeof ( float );
		for ( size_t i = 0; i != numFloats; i++ )
		{
			meshData_.vertexData_.push_back ( 0 );
		}
		vertexBufferSize = (vertexBufferSize + offsetAlignment) & ~(offsetAlignment - 1);
	}
	
	VulkanBuffer storage = ctx_.resources_.addStorageBuffer ( vertexBufferSize + indexBufferSize );
	uploadBufferData ( ctx_.vkDev_, storage.memory, 0, meshData_.vertexData_.data (), vertexBufferSize );
	uploadBufferData ( ctx_.vkDev_, storage.memory, vertexBufferSize, meshData_.indexData_.data (), indexBufferSize );

	vertexBuffer_ = BufferAttachment{
		.dInfo_ = {
			.type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderStageFlags_ = VK_SHADER_STAGE_VERTEX_BIT
		},
		.buffer_ = storage,
		.offset_ = 0,
		.size_ = vertexBufferSize
	};

	indexBuffer_ = BufferAttachment{
		.dInfo_ = {
			.type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderStageFlags_ = VK_SHADER_STAGE_VERTEX_BIT
		},
		.buffer_ = storage,
		.offset_ = vertexBufferSize,
		.size_ = indexBufferSize
	};
}

void VKSceneData::convertGlobalToShapeTransforms ()
{
	// fill the shapeTransforms_ array from globalTransforms_
	size_t i = 0; 
	for ( const auto& c : shapes_ )
	{
		shapeTransforms_[i++] = scene_.globalTransforms_[c.transformIndex];
	}
}

void VKSceneData::recalculateAllTransforms ()
{
	// force recalculation of global transformations
	markAsChanged ( scene_, 0 );
	recalculateGlobalTransforms ( scene_ );

}

void VKSceneData::uploadGlobalTransforms ()
{
	convertGlobalToShapeTransforms ();
	uploadBufferData ( ctx_.vkDev_, transforms_.memory, 0, shapeTransforms_.data (), transforms_.size );
}

void VKSceneData::updateMaterial ( int matIdx )
{
	uploadBufferData ( ctx_.vkDev_, material_.memory, matIdx * sizeof ( MaterialDescription ), materials_.data () + matIdx, sizeof ( MaterialDescription ) );
}

MultiRenderer::MultiRenderer ( 
	VulkanRenderContext& ctx,
	VKSceneData& sceneData, 
	const char* vtxShaderFile, 
	const char* fragShaderFile, 
	const std::vector<VulkanTexture>& outputs, 
	RenderPass screenRenderPass, 
	const std::vector<BufferAttachment>& auxBuffers, const std::vector<TextureAttachment>& auxTextures ) 
	: Renderer(ctx)
	, sceneData_(sceneData)
{
	printf ( "MultiRenderer: initRenderPass\n" );

	const PipelineInfo pInfo = initRenderPass ( PipelineInfo{}, outputs, screenRenderPass, ctx.screenRenderPass_ );

	const uint32_t indirectDataSize = static_cast<uint32_t>(sceneData_.shapes_.size () * sizeof ( VkDrawIndirectCommand ));

	const size_t imgCount = ctx.vkDev_.swapchainImages.size ();
	uniforms_.resize ( imgCount );
	shape_.resize ( imgCount );
	indirect_.resize ( imgCount );

	descriptorSets_.resize ( imgCount );

	const uint32_t shapeSize = static_cast<uint32_t>(sceneData_.shapes_.size () * sizeof ( DrawData ));
	const uint32_t uniformBufferSize = sizeof ( ubo_ );

	std::vector<TextureAttachment> textureAttachments;
	if ( sceneData_.envMap_.width ) textureAttachments.push_back ( fsTextureAttachment ( sceneData_.envMap_ ) );
	if ( sceneData_.envMapIrradiance_.width ) textureAttachments.push_back ( fsTextureAttachment ( sceneData_.envMapIrradiance_ ) );
	if ( sceneData_.brdfLUT_.width ) textureAttachments.push_back ( fsTextureAttachment ( sceneData_.brdfLUT_ ) );

	for ( const auto& t : auxTextures )
	{
		textureAttachments.push_back ( t );
	}

	DescriptorSetInfo dsInfo = {
		.buffers_ = {
			uniformBufferAttachment ( VulkanBuffer {},			0,		uniformBufferSize,						VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT ),
			sceneData_.vertexBuffer_,
			sceneData_.indexBuffer_,
			storageBufferAttachment ( VulkanBuffer {},			0,		shapeSize,								VK_SHADER_STAGE_VERTEX_BIT ),
			storageBufferAttachment ( sceneData_.material_,		0,		(uint32_t)sceneData_.material_.size,	VK_SHADER_STAGE_FRAGMENT_BIT ),
			storageBufferAttachment ( sceneData_.transforms_,	0,		(uint32_t)sceneData_.transforms_.size,	VK_SHADER_STAGE_VERTEX_BIT )
		},
		.textures_ = textureAttachments,
		.textureArrays_ = { sceneData_.allMaterialTextures_ }
	};
			
	for ( const auto& b : auxBuffers )
	{
		dsInfo.buffers_.push_back ( b );
	}

	printf ( "MultiRenderer: descriptor sets\n" );
	descriptorSetLayout_ = ctx.resources_.addDescriptorSetLayout ( dsInfo );
//	setVkDescriptorSetLayoutName ( ctx.vkDev_, &descriptorSetLayout_, "MultiRenderer::descriptorSetLayout" );
	setVkObjectName ( ctx.vkDev_.device, (uint64_t)descriptorSetLayout_, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "MultiRenderer::descriptorSetLayout" );
	descriptorPool_ = ctx.resources_.addDescriptorPool ( dsInfo, (uint32_t)imgCount );
//	setVkDescriptorPoolName ( ctx.vkDev_, &descriptorPool_, "MultiRenderer::descriptorPool" );
	setVkObjectName ( ctx.vkDev_.device, (uint64_t)descriptorPool_, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "MultiRenderer::descriptorPool" );

	for ( size_t i = 0; i != imgCount; i++ )
	{
		char uniformBufferName[64];
		char indirectBufferName[64];
		char storageBufferName[64];
		char descriptorSetName[64];

		sprintf ( uniformBufferName, "MultiRenderer::uniformBuffer %d", (int)i );
		sprintf ( indirectBufferName, "MultiRenderer::indirectBuffer %d", (int)i );
		sprintf ( storageBufferName, "MultiRenderer::storageBuffer %d", (int)i );
		sprintf ( descriptorSetName, "MultiRenderer::descriptorSet %d", (int)i );

		uniforms_[i] = ctx.resources_.addUniformBuffer ( uniformBufferSize );

	//	setVkBufferName ( ctx.vkDev_, &uniforms_[i].buffer, uniformBufferName );
	//	setVkObjectName ( ctx.vkDev_.device, (uint64_t)uniforms_[i].buffer, VK_OBJECT_TYPE_BUFFER, uniformBufferName );
		setVkObjectTag ( ctx.vkDev_.device, (uint64_t)uniforms_[i].buffer, VK_OBJECT_TYPE_BUFFER, (uint64_t)(1 + i * imgCount), sizeof ( uniformBufferName ) + 1, uniformBufferName );

		indirect_[i] = ctx.resources_.addIndirectBuffer ( indirectDataSize );

	//	setVkBufferName ( ctx.vkDev_, &indirect_[i].buffer, indirectBufferName );
	//	setVkObjectName ( ctx.vkDev_.device, (uint64_t)indirect_[i].buffer, VK_OBJECT_TYPE_BUFFER, indirectBufferName );
		setVkObjectTag ( ctx.vkDev_.device, (uint64_t)indirect_[i].buffer, VK_OBJECT_TYPE_BUFFER, (uint64_t)(2 + i * imgCount), sizeof ( indirectBufferName ) + 1, indirectBufferName );
		
		updateIndirectBuffers (i);

		shape_[i] = ctx.resources_.addStorageBuffer ( shapeSize );

	//	setVkBufferName ( ctx.vkDev_, &shape_[i].buffer, storageBufferName );
	//	setVkObjectName ( ctx.vkDev_.device, (uint64_t)shape_[i].buffer, VK_OBJECT_TYPE_BUFFER, storageBufferName );
		setVkObjectTag ( ctx.vkDev_.device, (uint64_t)shape_[i].buffer, VK_OBJECT_TYPE_BUFFER, (uint64_t)(3 + i * imgCount), sizeof ( storageBufferName ) + 1, storageBufferName );

		uploadBufferData ( ctx.vkDev_, shape_[i].memory, 0, sceneData_.shapes_.data (), shapeSize );

		dsInfo.buffers_[0].buffer_ = uniforms_[i];
		dsInfo.buffers_[3].buffer_ = shape_[i];

		descriptorSets_[i] = ctx.resources_.addDescriptorSet ( descriptorPool_, descriptorSetLayout_ );

	//	setVkObjectName ( ctx.vkDev_.device, (uint64_t)descriptorSets_[i], VK_OBJECT_TYPE_DESCRIPTOR_SET, descriptorSetName );
	//	setVkDescriptorSetName ( ctx.vkDev_, &descriptorSets_[i], descriptorSetName );
		setVkObjectTag ( ctx.vkDev_.device, (uint64_t)descriptorSets_[i], VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)(4 + i * imgCount), sizeof ( descriptorSetName ) + 1, descriptorSetName );

		ctx.resources_.updateDescriptorSet ( descriptorSets_[i], dsInfo );
	}

	printf ( "MultiRenderer: initPipeline\n" );

	initPipeline ( { vtxShaderFile, fragShaderFile }, pInfo );

//	setVkPipelineName ( ctx.vkDev_, &graphicsPipeline_, "MultiRenderer::graphicsPipeline" );
	setVkObjectName ( ctx.vkDev_.device, (uint64_t)graphicsPipeline_, VK_OBJECT_TYPE_PIPELINE, "MultiRenderer::graphicsPipeline" );

	printf ( "MultiRenderer: init done!\n" );

//#ifdef _DEBUG // add names and tags here
//	setVkPipelineName ( ctx.vkDev_, &graphicsPipeline_, "MultiRenderer::graphicsPipeline_" );
//	setVkPipelineLayoutName ( ctx.vkDev_, &pipelineLayout_, "MultiRenderer::pipelineLayout_" );
//	setVkDescriptorSetLayoutName ( ctx.vkDev_, &descriptorSetLayout_, "MultiRenderer::descriptorSetLayout_" );	
//#endif

//	initPipeline ( getShaderFilenamesWithRoot ( "assets/shaders/VK07_MultiRenderer.vert", "assets/shaders/VK07_MultiRenderer.frag" ), pInfo );
}

void MultiRenderer::fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp )
{
	beginRenderPass ( (rp != VK_NULL_HANDLE) ? rp : renderPass_.handle_, (fb != VK_NULL_HANDLE) ? fb : framebuffer_, cmdBuffer, currentImage );

	/* For CountKHR (Vulkan 1.1) we may use indirect rendering with GPU-based object counter */
	/// vkCmdDrawIndirectCountKHR(commandBuffer, indirectBuffers_[currentImage], 0, countBuffers_[currentImage], 0, shapes.size(), sizeof(VkDrawIndirectCommand));
	/* For Vulkan 1.0 vkCmdDrawIndirect is enough */
	vkCmdDrawIndirect ( cmdBuffer, indirect_[currentImage].buffer, 0, static_cast<uint32_t>(sceneData_.shapes_.size ()), sizeof ( VkDrawIndirectCommand ) );

	vkCmdEndRenderPass ( cmdBuffer );
}

void MultiRenderer::updateBuffers ( size_t currentImage )
{
	updateUniformBuffer ( (uint32_t)currentImage, 0, sizeof ( ubo_ ), &ubo_ );
}

void MultiRenderer::updateIndirectBuffers ( size_t currentImage, bool* visibility )
{
	VkDrawIndirectCommand* data = nullptr;
	vkMapMemory ( ctx_.vkDev_.device, indirect_[currentImage].memory, 0, sizeof ( VkDrawIndirectCommand ), 0, (void**)&data );

	const uint32_t size = static_cast<uint32_t>(sceneData_.shapes_.size ());

	for ( uint32_t i = 0; i != size; i++ )
	{
		const uint32_t j = sceneData_.shapes_[i].meshIndex;

		const uint32_t lod = sceneData_.shapes_[i].LOD;
		data[i] = {
			.vertexCount = sceneData_.meshData_.meshes_[j].getLODIndicesCount ( lod ),
			.instanceCount = visibility ? (visibility[i] ? 1u : 0u) : 1u,
			.firstVertex = 0,
			.firstInstance = i
		};
	}
	
	vkUnmapMemory ( ctx_.vkDev_.device, indirect_[currentImage].memory );
}

bool MultiRenderer::checkLoadedTextures ()
{
	VKSceneData::LoadedImageData data;

	{
		std::lock_guard lock ( sceneData_.loadedFilesMutex_ );

		if ( sceneData_.loadedFiles_.empty () )
		{
			return false;
		}
		
		data = sceneData_.loadedFiles_.back ();

		sceneData_.loadedFiles_.pop_back ();
	}

	this->updateTexture ( data.index_, ctx_.resources_.addRGBATexture ( data.w_, data.h_, const_cast<uint8_t*>(data.img_) ) );

	stbi_image_free ( (void*)data.img_ );
	return true;
}
