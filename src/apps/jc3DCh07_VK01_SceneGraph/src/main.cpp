//#include <jcVkFramework/vkFramework/VulkanApp.h>
//#include <jcVkFramework/vkFramework/GuiRenderer.h>
//#include <jcVkFramework/vkFramework/QuadRenderer.h>
//#include <jcVkFramework/vkFramework/MultiRenderer.h>
//#include <jcVkFramework/vkFramework/InfinitePlaneRenderer.h>
//
//#include <jcCommonFramework/ResourceString.h>
//
//#include "ImGuizmo.h"
//
//#include <math.h>
//
//using glm::mat4;
//using glm::mat3;
//using glm::vec4;
//using glm::vec3;
//using glm::vec2;
//
//struct MyApp : public CameraApp
//{
//private:
//	InfinitePlaneRenderer plane_;
//	GuiRenderer imgui_;
////	QuadRenderer quad_;
//
//	int selectedNode = -1;
//
//public:
//	MyApp() : CameraApp(1280, 720)
//		, plane_ ( ctx_ )
//		, imgui_(ctx_)
//	{
//		onScreenRenderers_.emplace_back ( plane_, false );
//		onScreenRenderers_.emplace_back ( imgui_, false );
//	}
//
//	void drawUI () override
//	{
//		ImGui::Begin ( "Information", nullptr );
//		ImGui::Text ( "FPS: %.2f", getFPS () );
//		ImGui::End ();
//
//		ImGui::Begin ( "Scene graph", nullptr );
//		
//	}
//};

#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkFramework/GuiRenderer.h>
#include <jcVkFramework/vkFramework/MultiRenderer.h>
#include <jcVkFramework/vkFramework/QuadRenderer.h>
#include <jcVkFramework/vkFramework/InfinitePlaneRenderer.h>

//#include <jcVkFramework/ResourceString.h>
#include <jcCommonFramework/ResourceString.h>

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
		, multiRenderer_(ctx_, sceneData_)
//		, multiRenderer_ ( ctx_, sceneData_, appendToRoot("assets/shaders/VK07_MultiRenderer.vert"), appendToRoot("assets/shaders/VK07_MultiRenderer.frag"))
		, imgui_ ( ctx_ )
	{
		onScreenRenderers_.emplace_back ( plane_, false );
		onScreenRenderers_.emplace_back ( multiRenderer_ );
		onScreenRenderers_.emplace_back ( imgui_, false );

		sceneData_.scene_.localTransforms_[0] = glm::rotate ( mat4 ( 1.0f ), (float)(M_PI / 2.0f), vec3 ( 1.0f, 0.0f, 0.0f ) );

#ifdef _DEBUG
		printf ( "Setting Vulkan Object Names...\n" );
		//setVkBufferName ( ctx_.vkDev_, &sceneData_.vertexBuffer_.buffer_.buffer, "MyApp::sceneData::vertexBuffer" );
		//setVkImageName ( ctx_.vkDev_, &envMap_.image.image, "MyApp::envMap" );
		//setVkImageName ( ctx_.vkDev_, &irrMap_.image.image, "MyApp::irrMap" );
		//setVkRenderPassName ( ctx_.vkDev_, &ctx_.clearRenderPass_.handle_, "Clear RenderPass" );
		//setVkRenderPassName ( ctx_.vkDev_, &ctx_.finalRenderPass_.handle_, "Final RenderPass" );
		//setVkRenderPassName ( ctx_.vkDev_, &ctx_.screenRenderPass_.handle_, "Screen RenderPass" );
		//setVkFramebufferName ( ctx_.vkDev_, &multiRenderer_.framebuffer_, "MyApp::multiRenderer::framebuffer" );
		//setVkFramebufferName ( ctx_.vkDev_, &plane_.framebuffer_, "MyApp::plane::framebuffer" );
		//setVkFramebufferName ( ctx_.vkDev_, &imgui_.framebuffer_, "MyApp::imgui::framebuffer" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)sceneData_.vertexBuffer_.buffer_.buffer, VK_OBJECT_TYPE_BUFFER, "MyApp::sceneData::vertetxBuffer" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)envMap_.image.image, VK_OBJECT_TYPE_IMAGE, "MyApp::sceneData::" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)irrMap_.image.image, VK_OBJECT_TYPE_IMAGE, "MyApp::irrMap" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.clearRenderPass_.handle_,VK_OBJECT_TYPE_RENDER_PASS, "Clear Render Pass" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.finalRenderPass_.handle_, VK_OBJECT_TYPE_RENDER_PASS, "Final Render Pass" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.screenRenderPass_.handle_, VK_OBJECT_TYPE_RENDER_PASS, "Screen Render Pass" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)multiRenderer_.framebuffer_, VK_OBJECT_TYPE_FRAMEBUFFER, "Multirenderer::framebuffer" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)plane_.framebuffer_, VK_OBJECT_TYPE_FRAMEBUFFER, "Multirenderer::framebuffer" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)imgui_.framebuffer_, VK_OBJECT_TYPE_FRAMEBUFFER, "ImGuiRenderer::framebuffer" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.device, VK_OBJECT_TYPE_DEVICE, "Vulkan Logical Device" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.physicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE, "Vulkan Physical Device" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "Vulkan Swapchain" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.commandPool, VK_OBJECT_TYPE_COMMAND_POOL, "Vulkan Command Pool" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.semaphore, VK_OBJECT_TYPE_SEMAPHORE, "Vulkan Semaphore" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.renderSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "Vulkan Render Semaphore" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.graphicsQueue, VK_OBJECT_TYPE_QUEUE, "Vulkan Graphics Queue" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.computeQueue, VK_OBJECT_TYPE_QUEUE, "Vulkan Compute Queue" );
		setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.computeCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "Vulkan Compute Command Buffer" );

		for ( size_t i = 0; i < ctx_.vkDev_.commandBuffers.size (); i++ )
		{
			std::string cmdBufferName = "Vulkan Command Bufffer[" + std::to_string ( i ) + "]";
			setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.commandBuffers[i], VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBufferName.c_str () );
		}

		for ( size_t i = 0; i < ctx_.vkDev_.swapchainImages.size (); i++ )
		{
			std::string swapChainName = "Vulkan Swapchain[" + std::to_string ( i ) + "]";
			setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.vkDev_.swapchainImages[i], VK_OBJECT_TYPE_IMAGE, swapChainName.c_str () );
		}

		for ( size_t i = 0; i < ctx_.resources_.allDSLayouts_.size (); i++ )
		{
			std::string dsLayoutName = "DescriptorSetLayout[" + std::to_string ( i ) + "]";
			setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.resources_.allDSLayouts_[i], VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, dsLayoutName.c_str () );
		}

		for ( size_t i = 0; i < ctx_.resources_.allDPools_.size(); i++ )
		{
			std::string dsPoolName = "DescriptorSetPool[" + std::to_string ( i ) + "]";
			setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.resources_.allDPools_[i], VK_OBJECT_TYPE_DESCRIPTOR_POOL, dsPoolName.c_str () );
		}

		for ( size_t i = 0; i < ctx_.resources_.allPipelineLayouts_.size(); i++ )
		{
			std::string pipelineLayoutName = "PipelineLayout[" + std::to_string ( i ) + "]";
			setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.resources_.allPipelineLayouts_[i], VK_OBJECT_TYPE_PIPELINE_LAYOUT, pipelineLayoutName.c_str () );
		}
		
		for ( size_t i = 0; i < ctx_.resources_.allPipelines_.size (); i++ )
		{
			std::string pipelineName = "Pipeline[" + std::to_string ( i ) + "]";
			setVkObjectName ( ctx_.vkDev_.device, (uint64_t)ctx_.resources_.allPipelines_[i], VK_OBJECT_TYPE_PIPELINE, pipelineName.c_str () );
		}
#endif
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
