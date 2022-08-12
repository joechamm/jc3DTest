#ifndef __SSAO_PROCESSOR_H__
#define __SSAO_PROCESSOR_H__

#include <jcVkFramework/vkFramework/CompositeRenderer.h>
#include <jcVkFramework/vkFramework/VulkanShaderProcessor.h>

#include <jcVkFramework/vkFramework/Barriers.h>

const int SSAOWidth = 0;
const int SSAOHeight = 0;

struct SSAOProcessor : public CompositeRenderer
{
	struct Params
	{
		float scale_ = 1.0f;
		float bias_ = 0.2f;
		float zNear_ = 0.1f;
		float zFar_ = 1000.0f;
		float radius_ = 0.2f;
		float attScale_ = 1.0f;
		float distScale_ = 0.5f;
	} *params_;

	

private:
	VulkanTexture rotateTex_;
	VulkanTexture SSAOTex_;
	VulkanTexture SSAOBlurXTex_;
	VulkanTexture SSAOBlurYTex_;

	BufferAttachment SSAOParamBuffer_;

	QuadProcessor SSAO_;
	QuadProcessor BlurX_;
	QuadProcessor BlurY_;
	QuadProcessor SSAOFinal_;

	ColorToShaderOptimalBarrier ssaoColorToShader_;
	ShaderOptimalToColorBarrier ssaoShaderToColor_;

	ColorToShaderOptimalBarrier blurXColorToShader_;
	ShaderOptimalToColorBarrier blurXShaderToColor_;

	ColorToShaderOptimalBarrier blurYColorToShader_;
	ShaderOptimalToColorBarrier blurYShaderToColor_;

	ColorToShaderOptimalBarrier finalColorToShader_;
	ShaderOptimalToColorBarrier finalShaderToColor_;
public:
	SSAOProcessor ( VulkanRenderContext& ctx, VulkanTexture colorTexture, VulkanTexture depthTexture, VulkanTexture outputTexture )
		: CompositeRenderer ( ctx )
		, rotateTex_ ( ctx_.resources_.loadTexture2D ( appendToRoot ( "assets/images/rot_texture.bmp" ).c_str () ) )
		, SSAOTex_ ( ctx_.resources_.addColorTexture ( SSAOWidth, SSAOHeight ) )
		, SSAOBlurXTex_ ( ctx_.resources_.addColorTexture ( SSAOWidth, SSAOHeight ) )
		, SSAOBlurYTex_ ( ctx_.resources_.addColorTexture ( SSAOWidth, SSAOHeight ) )
		, SSAOParamBuffer_ ( mappedUniformBufferAttachment ( ctx_.resources_, &params_, VK_SHADER_STAGE_FRAGMENT_BIT ) )
		, SSAO_ ( ctx_, DescriptorSetInfo {
			.buffers_ = { SSAOParamBuffer_ },
			.textures_ = { fsTextureAttachment ( depthTexture ), fsTextureAttachment ( rotateTex_ ) },
			}, { SSAOTex_ }, appendToRoot ( "assets/shaders/VK08_SSAO.frag" ).c_str () )
			, BlurX_ ( ctx_, DescriptorSetInfo {
				.textures_ = { fsTextureAttachment ( SSAOBlurXTex_ ) } }, { SSAOBlurXTex_ }, appendToRoot ( "assets/shaders/VK08_SSAOBlurX.frag" ).c_str () )
				, BlurY_ ( ctx_, DescriptorSetInfo {
					.textures_ = { fsTextureAttachment ( SSAOBlurXTex_ ) } }, { SSAOBlurYTex_ }, appendToRoot ( "assets/shaders/VK08_SSAOBlurY.frag" ).c_str () )
					, SSAOFinal_ ( ctx_, DescriptorSetInfo { .buffers_ = { SSAOParamBuffer_ }, .textures_ = { fsTextureAttachment ( colorTexture ), fsTextureAttachment ( SSAOBlurYTex_ ) } }, { outputTexture }, appendToRoot ( "assets/shaders/VK08_SSAOFinal.frag" ).c_str () )
		, ssaoColorToShader_ ( ctx_, SSAOTex_ )
		, ssaoShaderToColor_ ( ctx_, SSAOTex_ )
		, blurXColorToShader_ ( ctx_, SSAOBlurXTex_ )
		, blurXShaderToColor_ ( ctx_, SSAOBlurXTex_ )
		, blurYColorToShader_ ( ctx_, SSAOBlurYTex_ )
		, blurYShaderToColor_ ( ctx_, SSAOBlurYTex_ )
		, finalColorToShader_ ( ctx_, outputTexture )
		, finalShaderToColor_ ( ctx_, outputTexture )
	{
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)rotateTex_.image.image, VK_OBJECT_TYPE_IMAGE, "rotateTex" );
		setVkObjectName(ctx_.vkDev_.device, (uint64_t)SSAOTex_.image.image, VK_OBJECT_TYPE_IMAGE, "SSAO");
		setVkObjectName ( ctx_.vkDev_.device, ( uint64_t ) SSAOBlurXTex_.image.image, VK_OBJECT_TYPE_IMAGE, "SSAOBlurX" );
		setVkObjectName ( ctx_.vkDev_.device, ( uint64_t ) SSAOBlurYTex_.image.image, VK_OBJECT_TYPE_IMAGE, "SSAOBlurY" );
		
		renderers_.emplace_back ( ssaoShaderToColor_, false );
		renderers_.emplace_back ( blurXShaderToColor_, false );
		renderers_.emplace_back ( blurYShaderToColor_, false );
		renderers_.emplace_back ( finalShaderToColor_, false );

		renderers_.emplace_back ( SSAO_, false );
		renderers_.emplace_back ( ssaoColorToShader_, false );

		renderers_.emplace_back ( BlurX_, false );
		renderers_.emplace_back ( blurXColorToShader_, false );
		
		renderers_.emplace_back ( BlurY_, false );
		renderers_.emplace_back ( blurYColorToShader_, false );

		renderers_.emplace_back ( SSAOFinal_, false );
		renderers_.emplace_back ( finalColorToShader_, false );		
	}

	inline VulkanTexture getSSAO () const
	{
		return SSAOTex_;
	}

	inline VulkanTexture getBlurX () const
	{
		return SSAOBlurXTex_;
	}

	inline VulkanTexture getBlurY () const
	{
		return SSAOBlurYTex_;
	}			
};


#endif // !__SSAO_PROCESSOR_H__
