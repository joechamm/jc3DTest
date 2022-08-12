#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkFramework/GuiRenderer.h>
#include <jcVkFramework/vkFramework/MultiRenderer.h>
#include <jcVkFramework/vkFramework/QuadRenderer.h>

#include <jcCommonFramework/ResourceString.h>

std::string envMapFile = appendToRoot ( "assets/images/piazza_bologni_1k.hdr" );
std::string irrMapFile = appendToRoot ( "assets/images/piazza_bologni_1k_irradiance.hdr" );

#include <jcVkFramework/vkFramework/effects/SSAOProcessor.h>

#include <imgui/imgui_internal.h>

struct MyApp : public CameraApp
{
	bool enableSSAO_ = true;

private:
	VulkanTexture colorTex_;
	VulkanTexture depthTex_;
	VulkanTexture finalTex_;

	VKSceneData sceneData_;
	
	MultiRenderer multiRenderer_;

	SSAOProcessor SSAO_;
	
	QuadRenderer quads_;

	GuiRenderer imgui_;

public:
	MyApp () :
		CameraApp ( 1600, 900 )
		, colorTex_ ( ctx_.resources_.addColorTexture () )
		, depthTex_ ( ctx_.resources_.addDepthTexture () )
		, finalTex_ ( ctx_.resources_.addColorTexture () )
		, sceneData_ ( ctx_, appendToRoot ( "assets/meshes/test.meshes" ).c_str (), appendToRoot ( "assets/meshes/test.scene" ).c_str (), appendToRoot ( "assets/meshes/test.materials" ).c_str (), ctx_.resources_.loadCubeMap ( envMapFile.c_str () ), ctx_.resources_.loadCubeMap ( irrMapFile.c_str () ) )
		, multiRenderer_ ( ctx_, sceneData_, appendToRoot ( "assets/shaders/VK08_MultiRenderer.vert" ).c_str (), appendToRoot ( "assets/shaders/VK08_MultiRenderer.frag" ).c_str (), { colorTex_, depthTex_ }, ctx_.resources_.addRenderPass ( { colorTex_, depthTex_ }, RenderPassCreateInfo { .clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First | eRenderPassBit_Offscreen } ) )
		, SSAO_ ( ctx_, colorTex_, depthTex_, finalTex_ )
		, quads_ ( ctx_, { colorTex_, finalTex_ } )
		, imgui_ ( ctx_, { colorTex_, SSAO_.getBlurY () } )
	{
		positioner_ = CameraPositioner_FirstPerson ( vec3 ( -10.0f, -3.0f, 3.0f ), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
		
		onScreenRenderers_.emplace_back ( multiRenderer_ );
		onScreenRenderers_.emplace_back ( SSAO_ );
		
		onScreenRenderers_.emplace_back ( quads_, false );
		onScreenRenderers_.emplace_back ( imgui_, false );
	}

	void drawUI () override
	{
		ImGui::Begin ( "Control", nullptr );
		ImGui::Checkbox ( "Enable SSAO", &enableSSAO_ );
		ImGui::PushItemFlag ( ImGuiItemFlags_Disabled, !enableSSAO_ );
		ImGui::PushStyleVar ( ImGuiStyleVar_Alpha, ImGui::GetStyle ().Alpha * enableSSAO_ ? 1.0f : 0.2f );
		ImGui::SliderFloat ( "SSAO scale", &SSAO_.params_->scale_, 0.0f, 2.0f );
		ImGui::SliderFloat ( "SSAO bias", &SSAO_.params_->bias_, 0.0f, 0.3f );
		ImGui::PopItemFlag ();
		ImGui::PopStyleVar ();
		ImGui::Separator ();
		ImGui::SliderFloat ( "SSAO radius", &SSAO_.params_->radius_, 0.05f, 0.5f );
		ImGui::SliderFloat ( "SSAO attenuation scale", &SSAO_.params_->attScale_, 0.5f, 1.5f );
		ImGui::SliderFloat ( "SSAO distance scale", &SSAO_.params_->distScale_, 0.0f, 1.0f );
		ImGui::End ();

		if ( enableSSAO_ )
		{
			imguiTextureWindow ( "SSAO", 2 );
		}
	}

	void draw3D () override
	{
		multiRenderer_.setMatrices ( getDefaultProjection (), camera_.getViewMatrix () );

		quads_.clear ();
		quads_.quad ( -1.0f, -1.0f, 1.0f, 1.0f, enableSSAO_ ? 1 : 0 );
	}
};

int main ()
{
	MyApp app;
	app.mainLoop ();
	return 0;
}