#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkFramework/GuiRenderer.h>
#include <jcVkFramework/vkFramework/MultiRenderer.h>
#include <jcVkFramework/vkFramework/QuadRenderer.h>
#include <jcVkFramework/vkFramework/InfinitePlaneRenderer.h>

#include <jcVkFramework/ResourceString.h>

#include <jcVkFramework/vkFramework/VulkanResources.h>

#include "ImGuizmo.h"

#include <math.h>

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;

struct MyApp : public CameraApp
{
private:
	VulkanTexture envMap_;
	VulkanTexture irrMap_;

	VKSceneData sceneData_;
	
	InfinitePlaneRenderer plane_;
	MultiRenderer multiRenderer_;
	GuiRenderer imgui_;

	int selectedNode = -1;

public:
	MyApp ()
		: CameraApp ( 1280, 720 )
		, envMap_ ( ctx_.resources_.loadCubeMap ( appendToRoot ( "assets/images/piazza_bologni_1k.hdr" ).c_str () ) )
		, irrMap_ ( ctx_.resources_.loadCubeMap ( appendToRoot ( "assets/images/piazza_bologni_1k_irradiance.hdr" ).c_str () ) )
		, sceneData_ ( ctx_, appendToRoot ( "assets/meshes/test_graph.meshes" ).c_str (), appendToRoot ( "assets/meshes/test_graph.scene" ).c_str (), appendToRoot ( "assets/meshes/test_graph.materials" ).c_str (), envMap_, irrMap_ )
		, plane_ ( ctx_ )
		, multiRenderer_ ( ctx_, sceneData_, appendToRoot("assets/shaders/VK07_MultiRenderer.vert"), appendToRoot("assets/shaders/VK07_MultiRenderer.frag"))
		, imgui_ ( ctx_ )
	{
		onScreenRenderers_.emplace_back ( plane_, false );
		onScreenRenderers_.emplace_back ( multiRenderer_ );
		onScreenRenderers_.emplace_back ( imgui_, false );

		sceneData_.scene_.localTransforms_[0] = glm::rotate ( mat4 ( 1.0f ), (float)(M_PI / 2.0f), vec3 ( 1.0f, 0.0f, 0.0f ) );
	}

	void drawUI () override
	{
		ImGui::Begin ( "Information", nullptr );
		ImGui::Text ( "FPS: %.2f", getFPS () );
		ImGui::End ();

		ImGui::Begin ( "Scene graph", nullptr );
			int node = renderSceneTree ( sceneData_.scene_, 0 );
			if ( node > -1 ) selectedNode = node;
		
		ImGui::End ();
		
		editNode ( selectedNode );
	}

	void draw3D () override
	{
		const mat4 p = getDefaultProjection ();
		const mat4 view = camera_.getViewMatrix ();
		
		multiRenderer_.setMatrices ( p, view );
		plane_.setMatrices ( p, view, mat4 ( 1.0f ) );
	}

	void update ( float deltaSeconds ) override
	{
		CameraApp::update ( deltaSeconds );

		// update/upload matrices for individual scene nodes
		sceneData_.recalculateAllTransforms ();
		sceneData_.uploadGlobalTransforms ();
	}
	
private:
	void editNode ( int node )
	{
		if ( node < 0 ) return;

		mat4 cameraProjection = getDefaultProjection ();
		mat4 cameraView = camera_.getViewMatrix ();

		ImGuizmo::SetOrthographic ( false );
		ImGuizmo::BeginFrame ();

		std::string name = getNodeName ( sceneData_.scene_, node );
		std::string label = name.empty () ? (std::string ( "Node" ) + std::to_string ( node )) : name;
		label = "Node: " + label;

		ImGui::Begin ( "Editor" );
		ImGui::Text ( "%s", label.c_str () );

		mat4 globalTransform = sceneData_.scene_.globalTransforms_[node]; // fetch global transform
		mat4 srcTransform = globalTransform;
		mat4 localTransform = sceneData_.scene_.localTransforms_[node];

		ImGui::Separator ();
		ImGuizmo::SetID ( 1 );

		editTransform ( cameraView, cameraProjection, globalTransform );
		mat4 deltaTransform = glm::inverse ( srcTransform ) * globalTransform; // calculate delta for edited global transform
		sceneData_.scene_.localTransforms_[node] = localTransform * deltaTransform; // modify local transform 
		markAsChanged ( sceneData_.scene_, node );

		ImGui::Separator ();
		ImGui::Text ( "%s", "Material" );

		editMaterial ( node );
		ImGui::End ();		
	}

	void editTransform ( const mat4& view, const mat4& proj, mat4& matrix )
	{
		static ImGuizmo::OPERATION gizmoOperation ( ImGuizmo::TRANSLATE );

		if ( ImGui::RadioButton ( "Translate", gizmoOperation == ImGuizmo::TRANSLATE ) )
		{
			gizmoOperation = ImGuizmo::TRANSLATE;
		}

		ImGui::SameLine ();
		if ( ImGui::RadioButton ( "Rotate", gizmoOperation == ImGuizmo::ROTATE ) )
		{
			gizmoOperation = ImGuizmo::ROTATE;
		}

		ImGuiIO& io = ImGui::GetIO ();
		ImGuizmo::SetRect ( 0, 0, io.DisplaySize.x, io.DisplaySize.y );
		ImGuizmo::Manipulate ( glm::value_ptr ( view ), glm::value_ptr ( proj ), gizmoOperation, ImGuizmo::WORLD, glm::value_ptr ( matrix ) );		
	}

	void editMaterial ( int node )
	{
		if ( !sceneData_.scene_.materialForNode_.contains ( node ) ) return;

		auto matIdx = sceneData_.scene_.materialForNode_.at ( node );
		MaterialDescription& material = sceneData_.materials_[matIdx];

		float emissiveColor[4];
		memcpy ( emissiveColor, &material.emissiveColor_, sizeof ( gpuvec4 ) );
		gpuvec4 oldColor = material.emissiveColor_;
		ImGui::ColorEdit3 ( "Emissive color", emissiveColor );

		if ( memcmp ( emissiveColor, &oldColor, sizeof ( gpuvec4 ) ) )
		{
			memcpy ( &material.emissiveColor_, emissiveColor, sizeof ( gpuvec4 ) );
			sceneData_.updateMaterial ( matIdx );
		}
	}
};

int main ()
{
	MyApp app;
	app.mainLoop ();
	return 0;
}
