#ifndef __HDRPROCESSOR_H__
#define __HDRPROCESSOR_H__

#include <jcVkFramework/vkFramework/effects/LuminanceCalculator.h>
#include <jcVkFramework/vkFramework/Barriers.h>
#include <jcCommonFramework/ResourceString.h>

struct HDRUniformBuffer
{
	float exposure_;
	float maxWhite_;
	float bloomStrength_;
	float adaptationSpeed_;
};

/** Apply bloom to input buffer */
struct HDRProcessor : public CompositeRenderer
{
private:
	// Static texture with rotation pattern
	VulkanTexture streaksPatternTex_;

	// Texture with values above 1.0
	VulkanTexture brightnessTex_;

	// The ping-pong texture pair for adapted luminances
	VulkanTexture adaptedLuminanceTex1_;
	VulkanTexture adaptedLuminanceTex2_;

	// First pass of blurring
	VulkanTexture bloomX1Tex_;
	VulkanTexture bloomY1Tex_;

	// Second pass of blurring
	VulkanTexture bloomX2Tex_;
	VulkanTexture bloomY2Tex_;
	
	VulkanTexture streaks1Tex_;
	VulkanTexture streaks2Tex_;

	// composed source + brightness 
	VulkanTexture resultTex_;
	
	QuadProcessor brightness_;
	QuadProcessor bloomX1_;
	QuadProcessor bloomY1_;
	QuadProcessor bloomX2_;
	QuadProcessor bloomY2_;
	
	QuadProcessor streaks1_;
	QuadProcessor streaks2_;

	// Light Adaptation processing
	QuadProcessor adaptationEven_;
	QuadProcessor adaptationOdd_;

	ShaderOptimalToColorBarrier adaptation1ToColor_;
	ColorToShaderOptimalBarrier adaptation1ToShader_;

	ShaderOptimalToColorBarrier adaptation2ToColor_;
	ColorToShaderOptimalBarrier adaptation2ToShader_;

	// Final composition
	QuadProcessor composerEven_;
	QuadProcessor composerOdd_;

	// barriers
	ShaderOptimalToColorBarrier bloomX1ToColor_;
	ColorToShaderOptimalBarrier bloomX1ToShader_;
	ShaderOptimalToColorBarrier bloomX2ToColor_;
	ColorToShaderOptimalBarrier bloomX2ToShader_;
	
	ShaderOptimalToColorBarrier bloomY1ToColor_;
	ColorToShaderOptimalBarrier bloomY1ToShader_;
	ShaderOptimalToColorBarrier bloomY2ToColor_;
	ColorToShaderOptimalBarrier bloomY2ToShader_;

	ShaderOptimalToColorBarrier streaks1ToColor_;
	ColorToShaderOptimalBarrier streaks1ToShader_;
	ShaderOptimalToColorBarrier streaks2ToColor_;
	ColorToShaderOptimalBarrier streaks2ToShader_;

	ShaderOptimalToColorBarrier brightnessToColor_;
	ColorToShaderOptimalBarrier brightnessToShader_;

	ShaderOptimalToColorBarrier resultToColor_;
	ColorToShaderOptimalBarrier resultToShader_;	

public:
	HDRProcessor(VulkanRenderContext& c, VulkanTexture input, VulkanTexture avgLuminance, BufferAttachment uniformBuffer) 
		: CompositeRenderer(c)
		, brightnessTex_(c.resources_.addColorTexture(0, 0, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, adaptedLuminanceTex1_(c.resources_.addColorTexture(1, 1, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, adaptedLuminanceTex2_(c.resources_.addColorTexture(1, 1, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, bloomX1Tex_(c.resources_.addColorTexture(0, 0, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, bloomY1Tex_(c.resources_.addColorTexture(0, 0, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, bloomX2Tex_(c.resources_.addColorTexture(0, 0, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, bloomY2Tex_(c.resources_.addColorTexture(0, 0, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, streaks1Tex_(c.resources_.addColorTexture(0, 0, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, streaks2Tex_(c.resources_.addColorTexture(0, 0, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		// output is an 8-bit RGB framebuffer
		, streaksPatternTex_(c.resources_.loadTexture2D(appendToRoot("assets/images/StreaksRotationPattern.bmp").c_str()))
		, resultTex_(c.resources_.addColorTexture())
		, brightness_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( input ) } }, { brightnessTex_ }, appendToRoot ( "assets/shaders/VK10_BrightPass.frag" ).c_str () )
		, bloomX1_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( brightnessTex_ ) } }, { bloomX1Tex_ }, appendToRoot ( "assets/shaders/VK10_BloomX.frag" ).c_str () )
		, bloomY1_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( bloomX1Tex_ ) } }, { bloomY1Tex_ }, appendToRoot ( "assets/shaders/VK10_BloomY.frag" ).c_str () )
		, bloomX2_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( bloomY1Tex_ ) } }, { bloomX2Tex_ }, appendToRoot ( "assets/shaders/VK10_BloomX.frag" ).c_str () )
		, bloomY2_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( bloomX2Tex_ ) } }, { bloomY2Tex_ }, appendToRoot ( "assets/shaders/VK10_BloomY.frag" ).c_str () )
		, streaks1_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( bloomY2Tex_ ), fsTextureAttachment ( streaksPatternTex_ ) } }, { streaks1Tex_ }, appendToRoot ( "assets/shaders/VK10_Streaks.frag" ).c_str () )
		, streaks2_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( streaks1Tex_ ), fsTextureAttachment ( streaksPatternTex_ ) } }, { streaks2Tex_ }, appendToRoot ( "assets/shaders/VK10_Streaks.frag" ).c_str () )
		, adaptationEven_ ( c, DescriptorSetInfo { .buffers_ = { uniformBuffer }, .textures_ = { fsTextureAttachment ( avgLuminance ), fsTextureAttachment ( adaptedLuminanceTex1_ ) } }, { adaptedLuminanceTex2_ }, appendToRoot ( "assets/shaders/VK10_LightAdaptation.frag" ).c_str () )
		, adaptationOdd_ ( c, DescriptorSetInfo { .buffers_ = { uniformBuffer }, .textures_ = { fsTextureAttachment ( avgLuminance ), fsTextureAttachment ( adaptedLuminanceTex2_ ) } }, { adaptedLuminanceTex1_ }, appendToRoot ( "assets/shaders/VK10_LightAdaptation.frag" ).c_str () )
		, adaptation1ToColor_ ( c, adaptedLuminanceTex1_ )
		, adaptation1ToShader_ ( c, adaptedLuminanceTex1_ )
		, adaptation2ToColor_ ( c, adaptedLuminanceTex2_ )
		, adaptation2ToShader_ ( c, adaptedLuminanceTex2_ )
		, composerEven_ ( c, DescriptorSetInfo { .buffers_ = { uniformBuffer }, .textures_ = {fsTextureAttachment ( input ), fsTextureAttachment ( adaptedLuminanceTex2_ ), fsTextureAttachment ( streaks2Tex_ ) } }, { resultTex_ }, appendToRoot ( "assets/shaders/VK10_HDR.frag" ).c_str () )
		, composerOdd_ ( c, DescriptorSetInfo { .buffers_ = { uniformBuffer }, .textures_ = {fsTextureAttachment ( input ), fsTextureAttachment ( adaptedLuminanceTex1_ ), fsTextureAttachment ( streaks2Tex_ ) } }, { resultTex_ }, appendToRoot ( "assets/shaders/VK10_HDR.frag" ).c_str () )
		, bloomX1ToColor_ ( c, bloomX1Tex_ )
		, bloomX1ToShader_ ( c, bloomX1Tex_ )
		, bloomX2ToColor_ ( c, bloomX2Tex_ )
		, bloomX2ToShader_ ( c, bloomX2Tex_ )

		, bloomY1ToColor_ ( c, bloomY1Tex_ )
		, bloomY1ToShader_ ( c, bloomY1Tex_ )
		, bloomY2ToColor_ ( c, bloomY2Tex_ )
		, bloomY2ToShader_ ( c, bloomY2Tex_ )

		, streaks1ToColor_ ( c, streaks1Tex_ )
		, streaks1ToShader_ ( c, streaks1Tex_ )
		, streaks2ToColor_ ( c, streaks2Tex_ )
		, streaks2ToShader_ ( c, streaks2Tex_ )

		, brightnessToColor_ ( c, brightnessTex_ )
		, brightnessToShader_ ( c, brightnessTex_ )
		, resultToColor_ ( c, resultTex_ )
		, resultToShader_ ( c, resultTex_ )
	{
		renderers_.emplace_back ( brightnessToColor_, false );
		renderers_.emplace_back ( brightness_, false );
		renderers_.emplace_back ( brightnessToShader_, false );

		renderers_.emplace_back ( bloomX1ToColor_, false );
		renderers_.emplace_back ( bloomX1_, false );
		renderers_.emplace_back ( bloomX1ToShader_, false );

		renderers_.emplace_back ( bloomY1ToColor_, false );
		renderers_.emplace_back ( bloomY1_, false );
		renderers_.emplace_back ( bloomY1ToShader_, false );

		renderers_.emplace_back ( bloomX2ToColor_, false );
		renderers_.emplace_back ( bloomX2_, false );
		renderers_.emplace_back ( bloomX2ToShader_, false );
		
		renderers_.emplace_back ( bloomY2ToColor_, false );
		renderers_.emplace_back ( bloomY2_, false );
		renderers_.emplace_back ( bloomY2ToShader_, false );

		renderers_.emplace_back ( streaks1ToColor_, false );
		renderers_.emplace_back ( streaks1_, false );
		renderers_.emplace_back ( streaks1ToShader_, false );

		renderers_.emplace_back ( streaks2ToColor_, false );
		renderers_.emplace_back ( streaks2_, false );
		renderers_.emplace_back ( streaks2ToShader_, false );

		renderers_.emplace_back ( adaptation2ToColor_, false ); // 21
		renderers_.emplace_back ( adaptationEven_, false ); // 22 
		renderers_.emplace_back ( adaptation2ToShader_, false ); // 23

		renderers_.emplace_back ( adaptation1ToColor_, false ); // 24
		renderers_.emplace_back ( adaptationOdd_, false ); // 25
		renderers_.emplace_back ( adaptation1ToShader_, false ); // 26
		
		renderers_ [ 21 ].enabled_ = false; // disable adaptationProcessor at the beginning
		renderers_ [ 22 ].enabled_ = false; 
		renderers_ [ 23 ].enabled_ = false;

		renderers_.emplace_back ( resultToColor_, false ); // 27
		renderers_.emplace_back ( composerEven_, false ); // 28
		renderers_.emplace_back ( composerOdd_, false ); // 29
		renderers_ [ 29 ].enabled_ = false; // disable composerOdd at the beginning

		renderers_.emplace_back ( resultToShader_, false );

		// convert 32.0 to 55.10 fixed point format (half-float) manually for RGB channels, Set alpha to 1.0
		// const uint16_t brightPixel[4] = { 0x5400, 0x5400, 0x5400, 0x3C00 }; // 64.0 as initial value
		const uint16_t brightPixel [ 4 ] = { 0x5000, 0x5000, 0x5000, 0x3C00 }; // 32.0 as initial value
		updateTextureImage ( c.vkDev_, adaptedLuminanceTex1_.image.image, adaptedLuminanceTex1_.image.imageMemory, 1, 1, LuminosityFormat, 1, &brightPixel, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		updateTextureImage(c.vkDev_, adaptedLuminanceTex2_.image.image, adaptedLuminanceTex2_.image.imageMemory, 1, 1, LuminosityFormat, 1, &brightPixel, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
		
	void fillCommandBuffer ( VkCommandBuffer cmdBuffer, size_t currentImage, VkFramebuffer fb1 = VK_NULL_HANDLE, VkRenderPass rp1 = VK_NULL_HANDLE ) override
	{
		// Call base method 
		CompositeRenderer::fillCommandBuffer ( cmdBuffer, currentImage, fb1, rp1 );
		// Swap avgLuminance inputs for adaptation and composer
		static const std::vector<int> switchIndices { 21, 22, 23, 24, 25, 26, 28, 29 };
		for ( auto i : switchIndices )
		{
			renderers_ [ i ].enabled_ = !renderers_ [ i ].enabled_;
		}			
	}

	inline VulkanTexture getBloom1 () const { return bloomY1Tex_;  }
	inline VulkanTexture getBloom2 () const { return bloomY2Tex_; }
	inline VulkanTexture getBrightness () const { return brightnessTex_; }
	inline VulkanTexture getStreaks1 () const { return streaks1Tex_; }
	inline VulkanTexture getStreaks2 () const { return streaks2Tex_; }
	inline VulkanTexture getAdaptedLum1 () const { return adaptedLuminanceTex1_; }
	inline VulkanTexture getAdaptedLum2 () const { return adaptedLuminanceTex2_; }
	inline VulkanTexture getResult () const { return resultTex_; }		
};

#endif // !__HDRPROCESSOR_H__
