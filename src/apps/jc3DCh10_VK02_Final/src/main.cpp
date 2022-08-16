#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkFramework/GuiRenderer.h>
#include <jcVkFramework/vkFramework/QuadRenderer.h>
#include <jcVkFramework/vkFramework/CubeRenderer.h>
#include <jcVkFramework/vkFramework/LineCanvas.h>

#include <jcVkFramework/vkFramework/effects/SSAOProcessor.h>
#include <jcVkFramework/vkFramework/effects/HDRProcessor.h>

#include <jcCommonFramework/ResourceString.h>

#include "FinalRenderer.h"

const uint32_t TEX_RGB = ( 0x2 << 16 );

#include <imgui/imgui_internal.h>

float g_LightPhi = -15.0f;
float g_LightTheta = +30.0f;

struct MyApp : public CameraApp
{
	bool showPyramid_ = false;
	bool showHDRDebug_ = false;
	bool enableHDR_ = true;
	bool enableSSAO_ = true;
	bool showSSAODebug_ = false;
	bool showShadowBuffer_ = false;
	bool showLightFrustum_ = false;
	bool showObjectBoxes_ = false;

private:
	HDRUniformBuffer* hdrUniforms_;

	VulkanTexture colorTex_;
	VulkanTexture depthTex_;
	VulkanTexture finalTex_;

	VulkanTexture luminanceResult_;
	VulkanTexture envMap_;
	VulkanTexture irrMap_;

	VKSceneData sceneData_;
	
	CubemapRenderer cubeRenderer_;
	FinalRenderer finalRenderer_;

	LuminanceCalculator luminance_;

	BufferAttachment hdrUniformBuffer_;
	HDRProcessor hdr_;

	SSAOProcessor ssao_;

	std::vector<VulkanTexture> displayedTextureList_;

	QuadRenderer quads_;

	GuiRenderer imgui_;
	LineCanvas canvas_;

	BoundingBox bigBox_;

	ShaderOptimalToDepthBarrier toDepth_;
	ShaderOptimalToColorBarrier lumToColor_;

	ColorWaitBarrier lumWait_;

public:
	MyApp ()
		: CameraApp ( 1600, 900, { .vertexPipelineStoresAndAtomics_ = true, .fragmentStoresAndAtomics_ = true } )
		, colorTex_ ( ctx_.resources_.addColorTexture ( 0, 0, LuminosityFormat ) )
		, depthTex_ ( ctx_.resources_.addDepthTexture () )
		, finalTex_ ( ctx_.resources_.addColorTexture ( 0, 0, LuminosityFormat ) )
		, luminanceResult_ ( ctx_.resources_.addColorTexture ( 1, 1, LuminosityFormat ) )
		, envMap_ ( ctx_.resources_.loadCubeMap ( appendToRoot ( "assets/images/immenstadter_horn_2k.hdr" ).c_str () ) )
		, irrMap_ ( ctx_.resources_.loadCubeMap ( appendToRoot ( "assets/images/immenstadter_horn_2k_irradiance.hdr" ).c_str () ) )
		, sceneData_ ( ctx_, appendToRoot ( "assets/meshes/bistro_all.meshes" ).c_str (), appendToRoot ( "assets/meshes/bistro_all.scene" ).c_str (), appendToRoot ( "assets/meshes/bistro_all.materials" ).c_str (), envMap_, irrMap_, true )
		, cubeRenderer_ ( ctx_, envMap_, { colorTex_, depthTex_ }, ctx_.resources_.addRenderPass ( { colorTex_, depthTex_ }, RenderPassCreateInfo { .clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First | eRenderPassBit_Offscreen } ) )
		// Renderer with opaque/transparent object management and OIT composition
		, finalRenderer_ ( ctx_, sceneData_, { colorTex_, depthTex_ } )
		// tone mapping (gamma correction / exposure)
		, luminance_ ( ctx_, finalTex_, luminanceResult_ )
		, hdrUniformBuffer_ ( mappedUniformBufferAttachment ( ctx_.resources_, &hdrUniforms_, VK_SHADER_STAGE_FRAGMENT_BIT ) )
		, hdr_ ( ctx_, finalTex_, luminanceResult_, hdrUniformBuffer_ )
		, ssao_ ( ctx_, finalRenderer_.outputColor_ /*colorTex_ for no-HDR */, depthTex_, finalTex_ )
		, displayedTextureList_ ( {
			finalRenderer_.shadowDepth_, finalTex_, depthTex_, ssao_.getBlurY (),			// 0 - 3
			colorTex_, luminance_.getResult64 (),											// 4 - 5
			luminance_.getResult32 (), luminance_.getResult16 (), luminance_.getResult08 (),	// 6 - 8
			luminance_.getResult04 (), luminance_.getResult02 (), luminance_.getResult01 (),	// 9 - 11
			hdr_.getBloom1 (), hdr_.getBloom2 (), hdr_.getBrightness (), hdr_.getResult (),		// 12 - 15
			hdr_.getStreaks1 (), hdr_.getStreaks2 (),											// 16 - 17
			hdr_.getAdaptedLum1 (), hdr_.getAdaptedLum2 (),									// 18 - 19
			finalRenderer_.outputColor_
			} )
		, quads_ ( ctx_, displayedTextureList_ )
		, imgui_ ( ctx_, displayedTextureList_ )
		, canvas_ ( ctx_ )
		, lumToColor_ ( ctx_, colorTex_ )
		, toDepth_ ( ctx_, depthTex_ )
		, lumWait_ ( ctx_, colorTex_ )
	{
		positioner_ = CameraPositioner_FirstPerson ( vec3 ( -10.0f, -3.0f, 3.0f ), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );

		hdrUniforms_->bloomStrength_ = 1.1f;
		hdrUniforms_->maxWhite_ = 1.17f;
		hdrUniforms_->exposure_ = 0.9f;
		hdrUniforms_->adaptationSpeed_ = 0.1f;

		setVkObjectName ( ctx_.vkDev_.device, ( uint64_t ) colorTex_.image.image, VK_OBJECT_TYPE_IMAGE, "color" );
		setVkObjectName ( ctx_.vkDev_.device, ( uint64_t ) depthTex_.image.image, VK_OBJECT_TYPE_IMAGE, "depth" );
		setVkObjectName ( ctx_.vkDev_.device, ( uint64_t ) finalTex_.image.image, VK_OBJECT_TYPE_IMAGE, "final" );

		setVkObjectName ( ctx_.vkDev_.device, ( uint64_t ) finalRenderer_.shadowColor_.image.image, VK_OBJECT_TYPE_IMAGE, "shadowColor" );
		setVkObjectName ( ctx_.vkDev_.device, ( uint64_t ) finalRenderer_.shadowDepth_.image.image, VK_OBJECT_TYPE_IMAGE, "shadowDepth" );

		onScreenRenderers_.emplace_back ( cubeRenderer_ );				// 0
		onScreenRenderers_.emplace_back ( toDepth_, false );			// 1
		onScreenRenderers_.emplace_back ( lumToColor_, false );			// 2

		onScreenRenderers_.emplace_back ( finalRenderer_ );				// 3

		onScreenRenderers_.emplace_back ( ssao_ );						// 4

		onScreenRenderers_.emplace_back ( lumWait_, false );			// 5
		onScreenRenderers_.emplace_back ( luminance_, false );			// 6
		onScreenRenderers_.emplace_back ( hdr_, false );				// 7

		onScreenRenderers_.emplace_back ( quads_, false );				// 8
		onScreenRenderers_.emplace_back ( imgui_, false );				// 9

		onScreenRenderers_.emplace_back ( canvas_ );					// 10

		{
			std::vector<BoundingBox> reorderedBoxes;
			reorderedBoxes.reserve ( sceneData_.shapes_.size () );

			// pretransform bounding boxes to world space
			for ( const auto& c : sceneData_.shapes_ )
			{
				const mat4 model = sceneData_.scene_.globalTransforms_ [ c.transformIndex ];
				reorderedBoxes.push_back ( sceneData_.meshData_.boxes_ [ c.meshIndex ] );
				reorderedBoxes.back ().transform ( model );
			}

			bigBox_ = reorderedBoxes.front ();
			for ( const auto& b : reorderedBoxes )
			{
				bigBox_.combinePoint ( b.min_ );
				bigBox_.combinePoint ( b.max_ );
			}
		}
	}

	void drawUI () override
	{
		const float indentSize = 16.0f;

		ImGui::Begin ( "Control", nullptr );

		ImGui::Checkbox ( "Show object bounding boxes", &showObjectBoxes_ );
		ImGui::Checkbox ( "Render transparent objects", &finalRenderer_.renderTransparentObjects_ );

		ImGui::Text ( "HDR" );
		ImGui::Indent ( indentSize );

		ImGui::Checkbox ( "Show Pyramid", &showPyramid_ );
		ImGui::Checkbox ( "Show HDR Debug", &showHDRDebug_ );

		ImGui::SliderFloat ( "Exposure", &hdrUniforms_->exposure_, 0.1f, 2.0f );
		ImGui::SliderFloat ( "Max white", &hdrUniforms_->maxWhite_, 0.5f, 2.0f );
		ImGui::SliderFloat ( "Bloom strength", &hdrUniforms_->bloomStrength_, 0.0f, 2.0f );
		ImGui::SliderFloat ( "Adaptation speed", &hdrUniforms_->adaptationSpeed_, 0.01f, 0.5f );

		ImGui::Unindent ( indentSize );
		ImGui::Separator ();

		ImGui::Text ( "SSAO" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Show SSAO buffer", &showSSAODebug_ );

		ImGui::SliderFloat ( "SSAO scale", &ssao_.params_->scale_, 0.0f, 2.0f );
		ImGui::SliderFloat ( "SSAO bias", &ssao_.params_->bias_, 0.0f, 0.3f );
		ImGui::Separator ();
		ImGui::SliderFloat ( "SSAO radius", &ssao_.params_->radius_, 0.05f, 0.5f );
		ImGui::SliderFloat ( "SSAO attenuation scale", &ssao_.params_->attScale_, 0.5f, 1.5f );
		ImGui::SliderFloat ( "SSAO distance scale", &ssao_.params_->distScale_, 0.0f, 1.0f );

		ImGui::Unindent ( indentSize );
		ImGui::Separator ();

		ImGui::Text ( "Shadows:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Enable shadows", &finalRenderer_.enableShadows_ );

		ImGui::PushItemFlag ( ImGuiItemFlags_Disabled, !finalRenderer_.enableShadows_ );
		ImGui::PushStyleVar ( ImGuiStyleVar_Alpha, ImGui::GetStyle ().Alpha * finalRenderer_.enableShadows_ ? 1.0f : 0.2f );

		ImGui::Checkbox ( "Show shadow buffer", &showShadowBuffer_ );
		ImGui::Checkbox ( "Show light frustum", &showLightFrustum_ );

		ImGui::SliderFloat ( "Light Theta", &g_LightTheta, -85.0f, +85.0f );
		ImGui::SliderFloat ( "Light Phi", &g_LightPhi, -85.0f, +85.0f );

		ImGui::PopItemFlag ();
		ImGui::PopStyleVar ();
		ImGui::Unindent ( indentSize );

		ImGui::End ();

		if ( showPyramid_ )
		{
			ImGui::Begin ( "Pyramid", nullptr );

			ImGui::Text ( "HDRColor" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 5 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Lum64" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 6 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Lum32" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 7 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Lum16" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 8 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Lum08" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 9 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Lum04" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 10 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Lum02" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 11 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Lum01" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 12 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::End ();
		}

		if ( showHDRDebug_ )
		{
			ImGui::Begin ( "Adaptation", nullptr );
			ImGui::Text ( "Adapt1" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 19 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Adapt2" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 20 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::End ();

			ImGui::Begin ( "Debug", nullptr );
			ImGui::Text ( "Bloom1" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 13 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Bloom2" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 14 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Bright" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 15 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Result" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 16 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );

			ImGui::Text ( "Streaks1" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 17 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::Text ( "Streaks2" );
			ImGui::Image ( ( void* ) ( intptr_t ) ( 18 | TEX_RGB ), ImVec2 ( 128, 128 ), ImVec2 ( 0.0f, 1.0f ), ImVec2 ( 1.0f, 0.0f ) );
			ImGui::End ();
		}

		if ( finalRenderer_.enableShadows_ && showShadowBuffer_ )
			imguiTextureWindow ( "Shadow", 1 | ( 1 << 16 ) );

		if ( enableSSAO_ && showSSAODebug_ )
			imguiTextureWindow ( "SSAO", 4 );
	}

	void draw3D () override
	{
		const mat4 p = getDefaultProjection ();
		const mat4 view = camera_.getViewMatrix ();

		canvas_.setCameraMatrix ( p * view );
		canvas_.clear ();

		const mat4 rot1 = glm::rotate ( mat4 ( 1.0f ), glm::radians ( g_LightTheta ), vec3 ( 0.0f, 0.0f, 1.0f ) );
		const mat4 rot2 = glm::rotate ( rot1, glm::radians ( g_LightPhi ), vec3 ( 1.0f, 0.0f, 0.0f ) );
		vec3 lightDir = glm::normalize ( vec3 ( rot2 * vec4 ( 0.0f, -1.0f, 0.0f, 1.0f ) ) );
		const mat4 lightView = glm::lookAt ( vec3 ( 0.0f ), lightDir, vec3 ( 0.0f, 0.0f, 1.0f ) );

		const BoundingBox box = bigBox_.getTransformed ( lightView );
		const mat4 lightProj = finalRenderer_.enableShadows_ ? glm::ortho ( box.min_.x, box.max_.x, box.min_.y, box.max_.y, -box.max_.z, -box.min_.z ) : mat4 ( 0.0f );

		if ( finalRenderer_.enableShadows_ && showLightFrustum_ )
		{
			drawBox3d ( canvas_, glm::scale ( mat4 ( 1.0f ), vec3 ( 1.0f, -1.0f, 1.0f ) ), bigBox_, vec4 ( 0.0f, 0.0f, 0.0f, 1.0f ) );
			drawBox3d ( canvas_, mat4 ( 1.0f ), box, vec4 ( 1.0f, 0.0f, 0.0f, 1.0f ) );
			renderCameraFrustum ( canvas_, lightView, lightProj, vec4 ( 1.0f, 0.0f, 0.0f, 1.0f ) );

			canvas_.line ( vec3 ( 0.0f ), lightDir * 100.0f, vec4 ( 0.0f, 0.0f, 1.0f, 1.0f ) );
		}

		if ( showObjectBoxes_ )
		{
			for ( const auto& b : sceneData_.meshData_.boxes_ )
			{
				drawBox3d ( canvas_, glm::scale ( mat4 ( 1.0f ), vec3 ( 1.0f, -1.0f, 1.0f ) ), b, vec4 ( 0.0f, 1.0f, 0.0f, 1.0f ) );
			}
		}

		cubeRenderer_.setMatrices ( p, view );
		finalRenderer_.setMatrices ( p, view );
		finalRenderer_.setLightParameters ( lightProj, lightView );
		finalRenderer_.setCameraPosition ( positioner_.getPosition () );

		for ( int i = 0; i < 25; i++ )
		{
			finalRenderer_.checkLoadedTextures ();
		}

		quads_.clear ();
		quads_.quad ( -1.0f, enableHDR_ ? 1.0f : -1.0f, 1.0f, enableHDR_ ? -1.0f : 1.0f, 15 );
	}
};

int main ()
{
	MyApp app;
	app.mainLoop ();
	return 0;
}