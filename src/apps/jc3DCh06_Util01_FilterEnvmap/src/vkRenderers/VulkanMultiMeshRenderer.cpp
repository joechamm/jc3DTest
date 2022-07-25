#include "vkRenderers/VulkanMultiMeshRenderer.h"

bool MultiMeshRenderer::createDescriptorSet ( VulkanRenderDevice& vkDev )
{
	const std::array<VkDescriptorSetLayoutBinding, 5> bindings = {
		descriptorSetLayoutBinding ( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		/* vertices [part of this.storageBuffer] */
		descriptorSetLayoutBinding ( 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		/* indices [part of this.storageBuffer] */
		descriptorSetLayoutBinding ( 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		/* instance data [this.instanceBuffer] */
		descriptorSetLayoutBinding ( 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT ),
		/* material data [this.materialBuffer] */
		descriptorSetLayoutBinding ( 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT )
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

		const VkDescriptorBufferInfo bufferInfo = { uniformBuffers_[i], 0, sizeof ( mat4 ) };
		const VkDescriptorBufferInfo bufferInfo2 = { storageBuffer_, 0, maxVertexBufferSize_ };
		const VkDescriptorBufferInfo bufferInfo3 = { storageBuffer_, maxVertexBufferSize_, maxIndexBufferSize_ };
		const VkDescriptorBufferInfo bufferInfo4 = { instanceBuffers_[i], 0, maxInstanceSize_ };
		const VkDescriptorBufferInfo bufferInfo5 = { materialBuffer_, 0, maxMaterialSize_ };

		const std::array<VkWriteDescriptorSet, 5> descriptorWrites = {
			bufferWriteDescriptorSet ( ds, &bufferInfo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ),
			bufferWriteDescriptorSet ( ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ),
			bufferWriteDescriptorSet ( ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ),
			bufferWriteDescriptorSet ( ds, &bufferInfo4, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ),
			bufferWriteDescriptorSet ( ds, &bufferInfo5, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER )
		//  imageWriteDescriptorSet(ds, &imageInfo, 3)
		};

		vkUpdateDescriptorSets ( vkDev.device, static_cast<uint32_t>(descriptorWrites.size ()), descriptorWrites.data (), 0, nullptr );
	}

	return true;
}

void MultiMeshRenderer::updateUniformBuffer ( VulkanRenderDevice& vkDev, size_t currentImage, const mat4& m )
{
	uploadBufferData ( vkDev, uniformBuffersMemory_[currentImage], 0, glm::value_ptr ( m ), sizeof ( mat4 ) );
}

void MultiMeshRenderer::updateInstanceBuffer ( VulkanRenderDevice& vkDev, size_t currentImage, uint32_t instanceSize, const void* instanceData )
{
	uploadBufferData ( vkDev, instanceBuffersMemory_[currentImage], 0, instanceData, instanceSize );
}

void MultiMeshRenderer::updateGeometryBuffers ( VulkanRenderDevice& vkDev, uint32_t vertexCount, uint32_t indexCount, const void* vertices, const void* indices )
{
	/* Geometry data is uploaded into the GPU in two parts */
	uploadBufferData ( vkDev, storageBufferMemory_, 0, vertices, vertexCount );
	uploadBufferData ( vkDev, storageBufferMemory_, maxVertexBufferSize_, indices, indexCount );
}

void MultiMeshRenderer::updateIndirectBuffers ( VulkanRenderDevice& vkDev, size_t currentImage )
{
	/* 
	* A buffer required for indirect rendering is filled according to the loaded instances list. 
	* After mapping a region of GPU-visible memory into a CPU-accessible pointer, we iterate the instance array. 
	* For each loaded mesh instance, we fetch the vertex count and vertex offset within the data buffer 
	*/
	VkDrawIndirectCommand* data = nullptr;
	vkMapMemory ( vkDev.device, indirectBuffersMemory_[currentImage], /*offset*/ 0, /*size*/ 2 * sizeof ( VkDrawIndirectCommand ), /*flags*/ 0, (void**)&data );
	for ( uint32_t i = 0; i < maxInstances_; i++ )
	{
		const uint32_t j = instances_[i].meshIndex;
		data[i] = {
			.vertexCount = static_cast<uint32_t>(meshes_[j].getLODIndicesCount ( instances_[i].LOD ) / sizeof ( uint32_t )),
			// TODO: allow more than one instance count by making modifications to the instance buffer layout with transformation matrices
			.instanceCount = 1,
			// the vertex offset has to be recalculated into a float value count. As we have one instance, we just set the first and only firstInstance value to i
			.firstVertex = static_cast<uint32_t>(meshes_[j].streamOffset[0] / meshes_[j].streamElementSize[0]),
			.firstInstance = i
		};
	}
	vkUnmapMemory ( vkDev.device, indirectBuffersMemory_[currentImage] );
}

void MultiMeshRenderer::fillCommandBuffer ( VkCommandBuffer commandBuffer, size_t currentImage )
{
	beginRenderPass ( commandBuffer, currentImage );

	vkCmdDrawIndirect ( commandBuffer, indirectBuffers_[currentImage], 0, maxInstances_, sizeof ( VkDrawIndirectCommand ) );
	vkCmdEndRenderPass ( commandBuffer );
}

MultiMeshRenderer::~MultiMeshRenderer ()
{
	VkDevice device = vkDev_.device;
	vkDestroyBuffer ( device, storageBuffer_, nullptr );
	vkFreeMemory ( device, storageBufferMemory_, nullptr );
	for ( size_t i = 0; i < swapchainFramebuffers_.size (); i++ )
	{
		vkDestroyBuffer ( device, instanceBuffers_[i], nullptr );
		vkFreeMemory ( device, instanceBuffersMemory_[i], nullptr );
		vkDestroyBuffer ( device, indirectBuffers_[i], nullptr );
		vkFreeMemory ( device, indirectBuffersMemory_[i], nullptr );
	}
	vkDestroyBuffer ( device, materialBuffer_, nullptr );
	vkFreeMemory ( device, materialBufferMemory_, nullptr );
	destroyVulkanImage ( device, depthTexture_ );
}

void MultiMeshRenderer::loadInstanceData ( const char* instanceFilename )
{
	/* load a list of transformations for each mesh */
	FILE* f = fopen ( instanceFilename, "rb" );
	// TODO: implement error checking
	fseek ( f, 0, SEEK_END );
	size_t fsize = ftell ( f );
	fseek ( f, 0, SEEK_SET );
	/* calculate the number of instances in the file */
	maxInstances_ = static_cast<uint32_t>(fsize / sizeof ( DrawData ));
	instances_.resize ( maxInstances_ );
	/* load the instance data */
	if ( fread ( instances_.data (), sizeof ( DrawData ), maxInstances_, f ) != maxInstances_ )
	{
		printf ( "MultiMeshRenderer: unable to read instance data from %s\n", instanceFilename );
		exit ( 255 );
	}
	fclose ( f );
}

MeshFileHeader MultiMeshRenderer::loadMeshData ( const char* meshFilename )
{
	MeshFileHeader header;
	FILE* f = fopen ( meshFilename, "rb" );
	/* read the header */
	if ( fread ( &header, 1, sizeof ( header ), f ) != sizeof ( header ) )
	{
		printf ( "MultiMeshRenderer: unable to read mesh file header from %s\n", meshFilename );
		exit ( 255 );
	}

	meshes_.resize ( header.meshCount );
	/* read individual mesh descriptions */
	if ( fread ( meshes_.data (), sizeof ( Mesh ), header.meshCount, f ) != header.meshCount )
	{
		printf ( "MultiMeshRenderer: unable to read mesh descriptors from %s\n", meshFilename );
		exit ( 255 );
	}

	indexData_.resize ( static_cast<size_t>(header.indexDataSize / sizeof ( uint32_t )));
	vertexData_.resize ( static_cast<size_t>(header.vertexDataSize / sizeof ( float )) );
	/* read the mesh indices and vertex data */
	if ( (fread ( indexData_.data (), 1, header.indexDataSize, f ) != header.indexDataSize) ||
		(fread ( vertexData_.data (), 1, header.vertexDataSize, f ) != header.vertexDataSize) )
	{
		printf ( "MultiMeshRenderer: unable to read index/vertex data from %s\n", meshFilename );
		exit ( 255 );
	}

	fclose ( f );
	return header;
}

MultiMeshRenderer::MultiMeshRenderer (
	VulkanRenderDevice& vkDev,
	const char* meshFilename,
	const char* instanceFilename,
	const char* materialFilename,
	const char* vertShaderFilename,
	const char* fragShaderFilename ) : vkDev_ ( vkDev ), RendererBase ( vkDev, VulkanImage () )
{
	// create a render pass object
	if ( !createColorAndDepthRenderPass ( vkDev, false, &renderPass_, RenderPassCreateInfo () ) )
	{
		printf ( "MultiMeshRenderer: failed to create render pass\n" );
		exit ( EXIT_FAILURE );
	}

	// store the frame buffer dimensions 
	framebufferWidth_ = vkDev.framebufferWidth;
	framebufferHeight_ = vkDev.framebufferHeight;
	
	// allocate a depth buffer
	createDepthResources ( vkDev, framebufferWidth_, framebufferHeight_, depthTexture_ );

	// read the mesh and instance data
	loadInstanceData ( instanceFilename );
	MeshFileHeader header = loadMeshData ( meshFilename );
	const uint32_t indirectDataSize = maxInstances_ * sizeof ( VkDrawIndirectCommand );
	maxInstanceSize_ = maxInstances_ * sizeof ( DrawData );
	maxMaterialSize_ = 1024;

	// we need a copy of the instance and indirect rendering data for each of the swapchain images
	instanceBuffers_.resize ( vkDev.swapchainImages.size () );
	instanceBuffersMemory_.resize ( vkDev.swapchainImages.size () );
	indirectBuffers_.resize ( vkDev.swapchainImages.size () );
	indirectBuffersMemory_.resize ( vkDev.swapchainImages.size () );

	if ( !createBuffer ( vkDev.device, vkDev.physicalDevice, maxMaterialSize_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, materialBuffer_, materialBufferMemory_ ) )
	{
		printf ( "MultiMeshRenderer: cannot create material buffer\n" );
		exit ( EXIT_FAILURE );
	}

	// To allocate a descriptor set, we need to save the sizes of the index and vertex buffers
	maxVertexBufferSize_ = header.vertexDataSize;
	maxIndexBufferSize_ = header.indexDataSize;

	/* our descriptor set has two logical storage buffers for index and vertex data, so we pad the vertex data with zeros so that it has the necessary alignment properties */
	VkPhysicalDeviceProperties devProps;
	vkGetPhysicalDeviceProperties ( vkDev.physicalDevice, &devProps );
	const uint32_t offsetAlignment = devProps.limits.minStorageBufferOffsetAlignment;
	if ( (maxVertexBufferSize_ & (offsetAlignment - 1)) != 0 )
	{
		int floats = (offsetAlignment - (maxVertexBufferSize_ & (offsetAlignment - 1))) / sizeof ( float );
		for ( int ii = 0; ii < floats; ii++ )
		{
			vertexData_.push_back ( 0 );
		}
		maxVertexBufferSize_ = (maxVertexBufferSize_ + offsetAlignment) & ~(offsetAlignment - 1);
	}

	if ( !createBuffer ( vkDev.device, vkDev.physicalDevice, maxVertexBufferSize_ + maxIndexBufferSize_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, storageBuffer_, storageBufferMemory_ ) )
	{
		printf ( "MultiMeshRenderer: cannot create vertex/index buffer\n" );
		exit ( EXIT_FAILURE );
	}

	updateGeometryBuffers ( vkDev, header.vertexDataSize, header.indexDataSize, vertexData_.data (), indexData_.data () );

	for ( size_t i = 0; i < vkDev.swapchainImages.size (); i++ )
	{
		/* 
		* For debugging and code simplification, we allocate indirect draw data buffers as host-visible. If the instances are not changed by the CPU (e.g. static geometry), such buffers could be allocated 
		* on the GPU for better performance. However this requires you to allocate a staging buffer.
		*/
		if ( !createBuffer ( vkDev.device, vkDev.physicalDevice, indirectDataSize, 
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,  // | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, /* for debugging we make it host-visible */
			indirectBuffers_[i], indirectBuffersMemory_[i] ) )
		{
			printf ( "MultiMeshRenderer: cannot create indirect buffer\n" );
			exit ( EXIT_FAILURE );
		}

		updateIndirectBuffers ( vkDev, i );

		// TODO: maxInstanceSize_ might not be the right parameter to pass here
		if ( !createBuffer ( vkDev.device, vkDev.physicalDevice, maxInstanceSize_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, instanceBuffers_[i], instanceBuffersMemory_[i] ) )
		{
			printf ( "MultiMeshRenderer: cannot create instance buffer\n" );
			exit ( EXIT_FAILURE );
		}

		updateInstanceBuffer ( vkDev, i, maxInstanceSize_, instances_.data () );
	}

	if ( !createUniformBuffers ( vkDev, sizeof ( mat4 ) ) ||
		!createColorAndDepthFramebuffers ( vkDev, renderPass_, VK_NULL_HANDLE, swapchainFramebuffers_ ) ||
		!createDescriptorPool ( vkDev, 1, 4, 0, &descriptorPool_ ) ||
		!createDescriptorSet ( vkDev ) ||
		!createPipelineLayout ( vkDev.device, descriptorSetLayout_, &pipelineLayout_ ) ||
		!createGraphicsPipeline ( vkDev, renderPass_, pipelineLayout_, std::vector<const char*> ( { vertShaderFilename, fragShaderFilename } ), &graphicsPipeline_ ) )
	{
		printf ( "MultiMeshRenderer: failed to create pipeline\n" );
		fflush ( stdout );
		exit ( EXIT_FAILURE );
	}
}