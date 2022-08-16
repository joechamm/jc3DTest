#include "FinalRenderer.h"
#include <stb/stb_image.h>

BaseMultiRenderer::BaseMultiRenderer (
	VulkanRenderContext& ctx,
	VKSceneData& sceneData,
	const std::vector<int>& objectIndices,
	const char* vertShader,
	const char* fragShader,
	const std::vector<VulkanTexture>& outputs,
	RenderPass screenRenderPass,
	const std::vector<BufferAttachment>& auxBuffers,
	const std::vector<TextureAttachment>& auxTextures )
	: Renderer ( ctx )
	, sceneData_ ( sceneData )
	, indices_ ( objectIndices )
{
	const PipelineInfo pInfo = initRenderPass ( PipelineInfo {}, outputs, screenRenderPass, ctx.screenRenderPass_ );

	const uint32_t indirectDataSize = ( uint32_t ) sceneData_.shapes_.size () * sizeof ( VkDrawIndirectCommand );

	const size_t imgCount = ctx.vkDev_.swapchainImages.size ();
	uniforms_.resize ( imgCount );
	shape_.resize ( imgCount );
	indirect_.resize ( imgCount );
	
	descriptorSets_.resize ( imgCount );

	const uint32_t shapesSize = ( uint32_t ) sceneData_.shapes_.size () * sizeof ( DrawData );
	const uint32_t uniformBufferSize = sizeof ( ubo_ );
	const uint32_t materialBufferSize = ( uint32_t ) sceneData_.material_.size;
	const uint32_t transformBufferSize = ( uint32_t ) sceneData_.transforms_.size;

	std::vector<TextureAttachment> textureAttachments;
	if ( sceneData_.envMap_.width )
	{
		textureAttachments.push_back(fsTextureAttachment(sceneData_.envMap_));
	}
	if ( sceneData_.envMapIrradiance_.width )
	{
		textureAttachments.push_back(fsTextureAttachment(sceneData_.envMapIrradiance_));
	}
	if ( sceneData_.brdfLUT_.width )
	{
		textureAttachments.push_back ( fsTextureAttachment ( sceneData_.brdfLUT_ ) );
	}

	for ( const auto& t : auxTextures )
	{
		textureAttachments.push_back ( t );
	}

	DescriptorSetInfo dsInfo = {
		.buffers_ = {
			uniformBufferAttachment ( VulkanBuffer {}, 0, uniformBufferSize, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT ),
			sceneData_.vertexBuffer_,
			sceneData_.indexBuffer_,
			storageBufferAttachment ( VulkanBuffer {}, 0, shapesSize, VK_SHADER_STAGE_VERTEX_BIT ),
			storageBufferAttachment ( sceneData_.material_, 0, materialBufferSize, VK_SHADER_STAGE_FRAGMENT_BIT ),
			storageBufferAttachment ( sceneData_.transforms_, 0, transformBufferSize, VK_SHADER_STAGE_VERTEX_BIT ),
		}, 
		.textures_ = textureAttachments, 
		.textureArrays_ = { sceneData_.allMaterialTextures_ }
	};

	for(const auto& b : auxBuffers)
	{
		dsInfo.buffers_.push_back(b);
	}

	descriptorSetLayout_ = ctx_.resources_.addDescriptorSetLayout ( dsInfo );
	descriptorPool_ = ctx_.resources_.addDescriptorPool ( dsInfo, ( uint32_t ) imgCount );

	for ( size_t i = 0; i < imgCount; i++ )
	{
		uniforms_ [ i ] = ctx_.resources_.addUniformBuffer ( uniformBufferSize );
		indirect_ [ i ] = ctx_.resources_.addIndirectBuffer ( indirectDataSize );
		updateIndirectBuffers ( i );

		shape_ [ i ] = ctx_.resources_.addStorageBuffer ( shapesSize );
		uploadBufferData ( ctx_.vkDev_, shape_ [ i ].memory, 0, sceneData_.shapes_.data (), shapesSize );

		dsInfo.buffers_ [ 0 ].buffer_ = uniforms_ [ i ];
		dsInfo.buffers_ [ 3 ].buffer_ = shape_ [ i ];

		descriptorSets_ [ i ] = ctx_.resources_.addDescriptorSet ( descriptorPool_, descriptorSetLayout_ );
		ctx_.resources_.updateDescriptorSet ( descriptorSets_ [ i ], dsInfo );
	}

	std::vector<std::string> shaderFiles = { std::string ( vertShader ), std::string ( fragShader ) };
	
	initPipeline ( shaderFiles, pInfo );
}

void BaseMultiRenderer::fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb, VkRenderPass rp )
{
	beginRenderPass ( ( rp != VK_NULL_HANDLE ) ? rp : renderPass_.handle_, ( fb != VK_NULL_HANDLE ) ? fb : framebuffer_, cmdBuffer, currentImage );

	/* For CountKHR (Vulkan 1.1) we may use indirect rendering with GPU-based object counter */
	/// vkCmdDrawIndirectCountKHR(commandBuffer, indirectBuffers_[currentImage], 0, countBuffers_[currentImage], 0, shapes.size(), sizeof(VkDrawIndirectCommand));
	/* For Vulkan 1.0 vkCmdDrawIndirect is enough */

	vkCmdDrawIndirect ( cmdBuffer, indirect_ [ currentImage ].buffer, 0, ( uint32_t ) sceneData_.shapes_.size (), sizeof ( VkDrawIndirectCommand ) );

	vkCmdEndRenderPass ( cmdBuffer );
}

void BaseMultiRenderer::updateIndirectBuffers ( size_t currentImage, bool* visibility )
{
	VkDrawIndirectCommand* data = nullptr;
	vkMapMemory(ctx_.vkDev_.device, indirect_[currentImage].memory, 0, sizeof(VkDrawIndirectCommand), 0, ( void** ) &data);
	
	const uint32_t indirectDataSize = ( uint32_t ) indices_.size (); // (uint32_t)sceneData_.shapes_.size();

	for ( uint32_t i = 0; i != indirectDataSize; i++ )
	{
		const uint32_t j = sceneData_.shapes_ [ indices_ [ i ] ].LOD;
		
		const uint32_t lod = sceneData_.shapes_ [ indices_ [ i ] ].LOD;
		data [ i ] = {
			.vertexCount = sceneData_.meshData_.meshes_ [ j ].getLODIndicesCount ( lod ),
			.instanceCount = visibility ? ( visibility [ indices_ [ i ] ] ? 1u : 0u ) : 1u,
			.firstVertex = 0,
			.firstInstance = ( uint32_t ) indices_ [ i ]
		};
	}

	vkUnmapMemory ( ctx_.vkDev_.device, indirect_ [ currentImage ].memory );
}

bool FinalRenderer::checkLoadedTextures ()
{
	VKSceneData::LoadedImageData data;

	{
		std::lock_guard lock ( sceneData_.loadedFilesMutex_ );

		if ( sceneData_.loadedFiles_.empty () ) return false;

		data = sceneData_.loadedFiles_.back ();

		sceneData_.loadedFiles_.pop_back ();
	}

	auto newTexture = ctx_.resources_.addRGBATexture ( data.w_, data.h_, const_cast< uint8_t* >( data.img_ ) );
	
	transparentRenderer_.updateTexture ( data.index_, newTexture, 14 );
	opaqueRenderer_.updateTexture ( data.index_, newTexture, 11 );

	stbi_image_free ( ( void* ) data.img_ );

	return true;
}