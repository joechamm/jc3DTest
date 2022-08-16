#ifndef __LUMINANCE_CALCULATOR_H__
#define __LUMINANCE_CALCULATOR_H__

#include <jcVkFramework/vkFramework/CompositeRenderer.h>
#include <jcVkFramework/vkFramework/VulkanShaderProcessor.h>
#include <jcVkFramework/vkFramework/Barriers.h>

#include <jcCommonFramework/ResourceString.h>

const VkFormat LuminosityFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

const int LuminosityWidth = 64;
const int LuminosityHeight = 64;

struct LuminanceCalculator : public CompositeRenderer
{
private:
	VulkanTexture source_;
	
	VulkanTexture lumTex64_;
	VulkanTexture lumTex32_;
	VulkanTexture lumTex16_;
	VulkanTexture lumTex08_;
	VulkanTexture lumTex04_;
	VulkanTexture lumTex02_;
	VulkanTexture lumTex01_;

	QuadProcessor src_To_64_;
	QuadProcessor lum64_To_32_;
	QuadProcessor lum32_To_16_;
	QuadProcessor lum16_To_08_;
	QuadProcessor lum08_To_04_;
	QuadProcessor lum04_To_02_;
	QuadProcessor lum02_To_01_;
	
	ShaderOptimalToColorBarrier lum64ToColor_;
	ColorToShaderOptimalBarrier lum64ToShader_;
	ShaderOptimalToColorBarrier lum32ToColor_;
	ColorToShaderOptimalBarrier lum32ToShader_;
	ShaderOptimalToColorBarrier lum16ToColor_;
	ColorToShaderOptimalBarrier lum16ToShader_;
	ShaderOptimalToColorBarrier lum08ToColor_;
	ColorToShaderOptimalBarrier lum08ToShader_;
	ShaderOptimalToColorBarrier lum04ToColor_;
	ColorToShaderOptimalBarrier lum04ToShader_;
	ShaderOptimalToColorBarrier lum02ToColor_;
	ColorToShaderOptimalBarrier lum02ToShader_;
	ShaderOptimalToColorBarrier lum01ToColor_;
	ColorToShaderOptimalBarrier lum01ToShader_;	

public:
	LuminanceCalculator ( VulkanRenderContext& c, VulkanTexture sourceTex, VulkanTexture lumTex )
		: CompositeRenderer ( c )
		, source_ ( sourceTex )
		, lumTex64_ ( c.resources_.addColorTexture ( LuminosityWidth, LuminosityHeight, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE ) )
		, lumTex32_ ( c.resources_.addColorTexture ( LuminosityWidth / 2, LuminosityHeight / 2, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ) )
		, lumTex16_ ( c.resources_.addColorTexture ( LuminosityWidth / 4, LuminosityHeight / 4, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ) )
		, lumTex08_ ( c.resources_.addColorTexture ( LuminosityWidth / 8, LuminosityHeight / 8, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ) )
		, lumTex04_ ( c.resources_.addColorTexture ( LuminosityWidth / 16, LuminosityHeight / 16, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ) )
		, lumTex02_ ( c.resources_.addColorTexture ( LuminosityWidth / 32, LuminosityHeight / 32, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ) )
		, lumTex01_ ( lumTex )
		, src_To_64_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( source_ ) } }, { lumTex64_ }, appendToRoot ( "assets/shaders/VK_downscale2x2.frag" ).c_str () )
		, lum64_To_32_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( lumTex64_ ) } }, { lumTex32_ }, appendToRoot ( "assets/shaders/VK_downscale2x2.frag" ).c_str () )
		, lum32_To_16_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( lumTex32_ ) } }, { lumTex16_ }, appendToRoot ( "assets/shaders/VK_downscale2x2.frag" ).c_str () )
		, lum16_To_08_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( lumTex16_ ) } }, { lumTex08_ }, appendToRoot ( "assets/shaders/VK_downscale2x2.frag" ).c_str () )
		, lum08_To_04_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( lumTex08_ ) } }, { lumTex04_ }, appendToRoot ( "assets/shaders/VK_downscale2x2.frag" ).c_str () )
		, lum04_To_02_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( lumTex04_ ) } }, { lumTex02_ }, appendToRoot ( "assets/shaders/VK_downscale2x2.frag" ).c_str () )
		, lum02_To_01_ ( c, DescriptorSetInfo { .textures_ = { fsTextureAttachment ( lumTex02_ ) } }, { lumTex01_ }, appendToRoot ( "assets/shaders/VK_downscale2x2.frag" ).c_str () )
		, lum64ToColor_ ( c, lumTex64_ )
		, lum64ToShader_ ( c, lumTex64_ )
		, lum32ToColor_ ( c, lumTex32_ )
		, lum32ToShader_ ( c, lumTex32_ )
		, lum16ToColor_ ( c, lumTex16_ )
		, lum16ToShader_ ( c, lumTex16_ )
		, lum08ToColor_ ( c, lumTex08_ )
		, lum08ToShader_ ( c, lumTex08_ )
		, lum04ToColor_ ( c, lumTex04_ )
		, lum04ToShader_ ( c, lumTex04_ )
		, lum02ToColor_ ( c, lumTex02_ )
		, lum02ToShader_ ( c, lumTex02_ )
		, lum01ToColor_ ( c, lumTex01_ )
		, lum01ToShader_ ( c, lumTex01_ )
	{
		setVkObjectName ( c.vkDev_.device, (uint64_t)lumTex64_.image.image, VK_OBJECT_TYPE_IMAGE, "lum64" );
		setVkObjectName ( c.vkDev_.device, ( uint64_t ) lumTex32_.image.image, VK_OBJECT_TYPE_IMAGE, "lum32" );
		setVkObjectName ( c.vkDev_.device, ( uint64_t ) lumTex16_.image.image, VK_OBJECT_TYPE_IMAGE, "lum16" );
		setVkObjectName ( c.vkDev_.device, ( uint64_t ) lumTex08_.image.image, VK_OBJECT_TYPE_IMAGE, "lum08" );
		setVkObjectName ( c.vkDev_.device, ( uint64_t ) lumTex04_.image.image, VK_OBJECT_TYPE_IMAGE, "lum04" );
		setVkObjectName ( c.vkDev_.device, ( uint64_t ) lumTex02_.image.image, VK_OBJECT_TYPE_IMAGE, "lum02" );

		renderers_.emplace_back ( lum64ToColor_, false );
		renderers_.emplace_back ( src_To_64_, false );
		renderers_.emplace_back ( lum64ToShader_, false );

		renderers_.emplace_back ( lum32ToColor_, false );
		renderers_.emplace_back ( lum64_To_32_, false );
		renderers_.emplace_back ( lum32ToShader_, false );

		renderers_.emplace_back ( lum16ToColor_, false );
		renderers_.emplace_back ( lum32_To_16_, false );
		renderers_.emplace_back ( lum16ToShader_, false );

		renderers_.emplace_back ( lum08ToColor_, false );
		renderers_.emplace_back ( lum16_To_08_, false );
		renderers_.emplace_back ( lum08ToShader_, false );

		renderers_.emplace_back ( lum04ToColor_, false );
		renderers_.emplace_back ( lum08_To_04_, false );
		renderers_.emplace_back ( lum04ToShader_, false );

		renderers_.emplace_back ( lum02ToColor_, false );
		renderers_.emplace_back ( lum04_To_02_, false );
		renderers_.emplace_back ( lum02ToShader_, false );

		renderers_.emplace_back ( lum01ToColor_, false );
		renderers_.emplace_back ( lum02_To_01_, false );
		renderers_.emplace_back ( lum01ToShader_, false );
	}

	inline VulkanTexture getResult64 () const { return lumTex64_; }
	inline VulkanTexture getResult32 () const { return lumTex32_; }
	inline VulkanTexture getResult16 () const { return lumTex16_; }
	inline VulkanTexture getResult08 () const { return lumTex08_; }
	inline VulkanTexture getResult04 () const { return lumTex04_; }
	inline VulkanTexture getResult02 () const { return lumTex02_; }
	inline VulkanTexture getResult01 () const { return lumTex01_; }
};

#endif // !__LUMINANCE_CALCULATOR_H__
