#ifndef __FINAL_RENDERER_H__
#define __FINAL_RENDERER_H__

#include <jcVkFramework/vkFramework/MultiRenderer.h>
#include <jcVkFramework/vkFramework/effects/LuminanceCalculator.h>
#include <jcCommonFramework/ResourceString.h>

#include <algorithm>
#include <numeric>

const uint32_t ShadowSize = 8192;

/**
	The "finalized" variant of MultiRenderer
	For Chapter 10's demo we need to extract indirect buffer with rendering commands
	and expose it externally so that different parts of the same scene can be rendered
	(in our case - transparent and opaque objects)
	This is almost a line-by-line repetition of MultiRenderer class from vkFramework,
	but the additional 'objectIndices' parameter shows which scene items are rendered here
*/

struct BaseMultiRenderer : public Renderer
{
private:
	VKSceneData& sceneData_;
	
	std::vector<int> indices_;

	std::vector<VulkanBuffer> indirect_;
	std::vector<VulkanBuffer> shape_;

	struct UBO
	{
		mat4 proj_;
		mat4 view_;
		vec4 cameraPos_;
	} ubo_;

public:
	BaseMultiRenderer (
		VulkanRenderContext& ctx,
		VKSceneData& sceneData,
		// indices of objects from sceneData's shape list
		const std::vector<int>& objectIndices,
		const char* vtxShaderFile = DefaultMeshVertexShader,
		const char* fragShaderFile = DefaultMeshFragmentShader,
		const std::vector<VulkanTexture>& outputs = std::vector<VulkanTexture> {},
		RenderPass screenRenderPass = RenderPass (),
		const std::vector<BufferAttachment>& auxBuffers = std::vector<BufferAttachment> {},
		const std::vector<TextureAttachment>& auxTextures = std::vector<TextureAttachment> {} );

	void updateIndirectBuffers ( size_t currentImage, bool* visibility = nullptr );

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE ) override;
	void updateBuffers ( size_t currentImage ) override
	{
		updateUniformBuffer ( ( uint32_t ) currentImage, 0, sizeof ( ubo_ ), &ubo_ );
	}

	inline void setMatrices ( const mat4& proj, const mat4& view )
	{
		const mat4 m1 = glm::scale ( mat4 ( 1.0f ), vec3 ( 1.0f, -1.0f, 1.0f ) );
		ubo_.proj_ = proj;
		ubo_.view_ = view * m1;
	}

	inline void setCameraPos(const vec3& cameraPos)
	{
		ubo_.cameraPos_ = vec4(cameraPos, 1.0f);
	}

	inline const VKSceneData& getSceneData() const 
	{ 
		return sceneData_;
	}
};

// Extract a list of indices of opaque objects
inline std::vector<int> getOpaqueIndices ( const VKSceneData& sd )
{
	std::vector<int> src_indices(sd.shapes_.size());
	std::iota ( src_indices.begin (), src_indices.end (), 0 );
	
	std::vector<int> list;

	std::copy_if ( src_indices.begin (), src_indices.end (), std::back_inserter ( list ),
		[&sd] ( const auto& idx )
		{
			const auto c = sd.shapes_ [ idx ]; // DrawData
			const auto& mtl = sd.materials_ [ c.materialIndex ];
			return ( mtl.flags_ & sMaterialFlags_Transparent ) == 0;
		} );

	return list;			
}

// Extract a list of transparent objects
inline std::vector<int> getTransparentIndices ( const VKSceneData& sd )
{
	std::vector<int> src_indices(sd.shapes_.size());
	std::iota ( src_indices.begin (), src_indices.end (), 0 );
	
	std::vector<int> list;

	std::copy_if ( src_indices.begin (), src_indices.end (), std::back_inserter ( list ),
		[&sd] ( const auto& idx )
		{
			const auto c = sd.shapes_ [ idx ]; // DrawData
			const auto& mtl = sd.materials_ [ c.materialIndex ];
			return ( mtl.flags_ & sMaterialFlags_Transparent ) > 0;
		} );

	return list;			
}

struct LightParamsBuffer
{
	mat4 proj;
	mat4 view;

	uint32_t width;
	uint32_t height;
};

// Single item in the OIT buffer. See Chapter 10's GL03_OIT demo and "Order-independent Transparency" Recipe in the book
struct TransparentFragment
{
	vec4 color;
	float depth;
	uint32_t next;
};

/**
	This the final variant of the scene rendering class
	It manages lists of opaque/transparent objects and uses two BaseMultiRenderer instances
	The transparent objects renderer fills the auxilliary OIT linked list buffer (see VK02_Glass.frag shader)
	and clears this per-pixel transparent fragment linked list at each frame
	OIT buffer composition is also performed by this class
	Additionally, this class provides boolean flags to enable/disable shadows and transparent objects
	Technically, this can be implemented as a CompositeRenderer
	and OIT can be extracted to a separate class,
	but it also would require a few more barrier classes, so we opted for a straight-forward solution
	similar to a list of OpenGL commands
*/

struct FinalRenderer : public Renderer
{
	VulkanTexture shadowColor_;
	VulkanTexture shadowDepth_;

	VulkanBuffer lightParams_;

	VulkanBuffer atomicBuffer_;
	VulkanBuffer headsBuffer_;
	VulkanBuffer oitBuffer_;

	VulkanTexture outputColor_;

	bool enableShadows_ = true;
	bool renderTransparentObjects_ = true;

private:

	VKSceneData& sceneData_;

	BaseMultiRenderer opaqueRenderer_;
	BaseMultiRenderer transparentRenderer_;

	BaseMultiRenderer shadowRenderer_;

	ShaderOptimalToColorBarrier colorToAttachment_;
	ShaderOptimalToDepthBarrier depthToAttachment_;

	VulkanBuffer whBuffer_;

	QuadProcessor clearOIT_;
	QuadProcessor composeOIT_;

	struct UBO
	{
		uint32_t width_;
		uint32_t height_;
	} ubo_;

	ShaderOptimalToColorBarrier outputToAttachment_;
	ColorToShaderOptimalBarrier outputToShader_;

public:
	FinalRenderer (
		VulkanRenderContext& ctx,
		VKSceneData& sceneData,
		const std::vector<VulkanTexture>& outputs = std::vector<VulkanTexture> {} )
		: Renderer ( ctx )
		, shadowColor_ ( ctx_.resources_.addColorTexture ( ShadowSize, ShadowSize ) )
		, shadowDepth_ ( ctx_.resources_.addDepthTexture ( ShadowSize, ShadowSize ) )
		, lightParams_ ( ctx_.resources_.addStorageBuffer ( sizeof ( LightParamsBuffer ) ) )
		, atomicBuffer_ ( ctx_.resources_.addStorageBuffer ( sizeof ( uint32_t ) ) )
		, headsBuffer_ ( ctx_.resources_.addStorageBuffer ( ctx.vkDev_.framebufferWidth* ctx.vkDev_.framebufferHeight * sizeof ( uint32_t ) ) )
		, oitBuffer_ ( ctx_.resources_.addStorageBuffer ( ctx.vkDev_.framebufferWidth* ctx.vkDev_.framebufferHeight * sizeof ( TransparentFragment ) ) )
		, outputColor_ ( ctx_.resources_.addColorTexture ( 0, 0, LuminosityFormat ) )
		, sceneData_ ( sceneData )
		, opaqueRenderer_ ( ctx, sceneData, getOpaqueIndices ( sceneData ), appendToRoot ( "assets/shaders/VK10_Shadow.vert" ).c_str (), appendToRoot ( "assets/shaders/VK10_Shadow.frag" ).c_str (), outputs, ctx_.resources_.addRenderPass ( outputs, RenderPassCreateInfo { .clearColor_ = false, .clearDepth_ = false, .flags_ = eRenderPassBit_Offscreen } )
			, { storageBufferAttachment ( lightParams_, 0, sizeof ( LightParamsBuffer ), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT ) }, { fsTextureAttachment ( shadowDepth_ ) } )
		, transparentRenderer_ ( ctx, sceneData, getTransparentIndices ( sceneData ), appendToRoot ( "assets/shaders/VK10_Shadow.vert" ).c_str (), appendToRoot ( "assets/shaders/VK10_Glass.frag" ).c_str (), outputs, ctx_.resources_.addRenderPass ( outputs, RenderPassCreateInfo {
			.clearColor_ = false, .clearDepth_ = false, .flags_ = eRenderPassBit_Offscreen } )
			, { storageBufferAttachment ( lightParams_, 0, sizeof ( LightParamsBuffer ), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT ),
				storageBufferAttachment ( atomicBuffer_, 0, sizeof ( uint32_t ), VK_SHADER_STAGE_FRAGMENT_BIT ),
				storageBufferAttachment ( headsBuffer_, 0, ctx.vkDev_.framebufferWidth * ctx.vkDev_.framebufferHeight * sizeof ( uint32_t ), VK_SHADER_STAGE_FRAGMENT_BIT ),
				storageBufferAttachment ( oitBuffer_, 0, ctx.vkDev_.framebufferWidth * ctx.vkDev_.framebufferHeight * sizeof ( TransparentFragment ), VK_SHADER_STAGE_FRAGMENT_BIT ) }
			, { fsTextureAttachment ( shadowDepth_ ) } )
		, shadowRenderer_ ( ctx, sceneData, getOpaqueIndices ( sceneData ), appendToRoot ( "assets/shaders/VK10_Depth.vert" ).c_str (), appendToRoot ( "assets/shaders/VK10_Depth.frag" ).c_str (), { shadowColor_, shadowDepth_ }, ctx_.resources_.addRenderPass ( { shadowColor_, shadowDepth_ }, RenderPassCreateInfo { .clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First | eRenderPassBit_Offscreen } ) )
		, colorToAttachment_ ( ctx, outputs [ 0 ] )
		, depthToAttachment_ ( ctx, outputs [ 1 ] )
		, whBuffer_ ( ctx_.resources_.addUniformBuffer ( sizeof ( UBO ) ) )
		, clearOIT_ ( ctx_, { .buffers_ = {
				uniformBufferAttachment ( whBuffer_, 0, sizeof ( ubo_ ), VK_SHADER_STAGE_FRAGMENT_BIT ),
				storageBufferAttachment ( headsBuffer_, 0, ctx.vkDev_.framebufferWidth * ctx.vkDev_.framebufferHeight * sizeof ( uint32_t ), VK_SHADER_STAGE_FRAGMENT_BIT )
			}, .textures_ = { fsTextureAttachment ( outputs [ 0 ] ) } },
			{ outputColor_ }, appendToRoot ( "assets/shaders/VK10_ClearBuffer.frag" ).c_str () )
		, composeOIT_ ( ctx_, { .buffers_ = {
				uniformBufferAttachment ( whBuffer_, 0, sizeof ( ubo_ ), VK_SHADER_STAGE_FRAGMENT_BIT ),
				storageBufferAttachment ( headsBuffer_, 0, ctx.vkDev_.framebufferWidth * ctx.vkDev_.framebufferHeight * sizeof ( uint32_t ), VK_SHADER_STAGE_FRAGMENT_BIT ),
				storageBufferAttachment ( oitBuffer_, 0, ctx.vkDev_.framebufferWidth * ctx.vkDev_.framebufferHeight * sizeof ( TransparentFragment ), VK_SHADER_STAGE_FRAGMENT_BIT )
				}, .textures_ = { fsTextureAttachment ( outputs [ 0 ] ) } }, { outputColor_ }, appendToRoot ( "assets/shaders/VK10_ComposeOIT.frag" ).c_str () )
		, outputToAttachment_ ( ctx_, outputColor_ )
		, outputToShader_ ( ctx_, outputColor_ )
	{
		ubo_.width_ = ctx.vkDev_.framebufferWidth;
		ubo_.height_ = ctx.vkDev_.framebufferHeight;

		setVkObjectName ( ctx_.vkDev_.device, ( uint64_t ) outputColor_.image.image, VK_OBJECT_TYPE_IMAGE, "outputColor" );
	}

	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE ) override
	{
		outputToAttachment_.fillCommandBuffer ( cmdBuffer, currentImage );
		
		clearOIT_.fillCommandBuffer ( cmdBuffer, currentImage );
		
		if ( enableShadows_ )
		{
			shadowRenderer_.fillCommandBuffer ( cmdBuffer, currentImage );
		}

		opaqueRenderer_.fillCommandBuffer ( cmdBuffer, currentImage );

		VkBufferMemoryBarrier headsBufferBarrier = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_HOST_READ_BIT,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = headsBuffer_.buffer,
			.offset = 0,
			.size = headsBuffer_.size
		};

		vkCmdPipelineBarrier ( cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &headsBufferBarrier, 0, nullptr );

		if ( renderTransparentObjects_ )
		{
			colorToAttachment_.fillCommandBuffer ( cmdBuffer, currentImage );
			depthToAttachment_.fillCommandBuffer ( cmdBuffer, currentImage );

			transparentRenderer_.fillCommandBuffer ( cmdBuffer, currentImage );

			VkMemoryBarrier readoutBarrier2 = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_HOST_READ_BIT
			};

			vkCmdPipelineBarrier ( cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &readoutBarrier2, 0, nullptr, 0, nullptr );
		}

		composeOIT_.fillCommandBuffer ( cmdBuffer, currentImage );
		outputToShader_.fillCommandBuffer ( cmdBuffer, currentImage );
	}

	void updateBuffers ( size_t currentImage ) override
	{
		transparentRenderer_.updateBuffers ( currentImage );
		opaqueRenderer_.updateBuffers ( currentImage );

		shadowRenderer_.updateBuffers ( currentImage );

		uint32_t zeroCount = 0;
		uploadBufferData ( ctx_.vkDev_, atomicBuffer_.memory, 0, &zeroCount, sizeof ( uint32_t ) );
		uploadBufferData(ctx_.vkDev_, whBuffer_.memory, 0, &ubo_, sizeof(ubo_));
	}

//	void updateIndirectBuffers ( size_t currentImage, bool* visibility = nullptr );

	inline void setMatrices ( const mat4& proj, const mat4& view )
	{
		transparentRenderer_.setMatrices ( proj, view );
		opaqueRenderer_.setMatrices ( proj, view );
	}

	inline void setLightParameters ( const mat4& lightProj, const mat4& lightView )
	{
		LightParamsBuffer lightParamsBuffer = {
			.proj = lightProj,
			.view = lightView,
			.width = ctx_.vkDev_.framebufferWidth,
			.height = ctx_.vkDev_.framebufferHeight
		};

		uploadBufferData ( ctx_.vkDev_, lightParams_.memory, 0, &lightParamsBuffer, sizeof ( LightParamsBuffer ) );

		shadowRenderer_.setMatrices ( lightProj, lightView );
	}

	inline void setCameraPosition ( const vec3& cameraPos )
	{
		transparentRenderer_.setCameraPos ( cameraPos );
		opaqueRenderer_.setCameraPos ( cameraPos );
		shadowRenderer_.setCameraPos ( cameraPos );
	}

	inline const VKSceneData& getSceneData () const
	{
		return sceneData_;
	}

	bool checkLoadedTextures ();
};
	

#endif // __FINAL_RENDERER_H__