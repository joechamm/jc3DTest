#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkFramework/GuiRenderer.h>
#include <jcVkFramework/vkFramework/MultiRenderer.h>

#include <jcCommonFramework/ResourceString.h>

struct MyApp : public CameraApp
{
private:
	VulkanTexture envMap_;
	VulkanTexture irrMap_;

	VKSceneData sceneData_;
	VKSceneData sceneData2_;

	MultiRenderer multiRenderer_;
	MultiRenderer multiRenderer2_;

	GuiRenderer imgui_;

public:
	MyApp () :
		CameraApp ( 1280, 720 )
		, envMap_ ( ctx_.resources_.loadCubeMap ( appendToRoot ( "assets/images/piazza_bologni_1k.hdr" ).c_str () ) )
		, irrMap_ ( ctx_.resources_.loadCubeMap ( appendToRoot ( "assets/images/piazza_bologni_1k_irradiance.hdr" ).c_str () ) )
		, sceneData_ ( ctx_, appendToRoot ( "assets/meshes/test.meshes" ), appendToRoot ( "assets/meshes/test.scene" ), appendToRoot ( "assets/meshes/test.materials" ), envMap_, irrMap_ )
		, sceneData2_ ( ctx_, appendToRoot ( "assets/meshes/test2.meshes" ), appendToRoot ( "assets/meshes/test2.scene" ), appendToRoot ( "assets/meshes/test2.materials" ), envMap_, irrMap_ )
		, multiRenderer_ ( ctx_, sceneData_ )
		, multiRenderer2_ ( ctx_, sceneData2_ )
		, imgui_ ( ctx_ )
	{
		positioner_ = CameraPositioner_FirstPerson ( vec3 ( -10.0f, -3.0f, 3.0f ), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );

		onScreenRenderers_.emplace_back ( multiRenderer_ );
		onScreenRenderers_.emplace_back ( multiRenderer2_ );
		onScreenRenderers_.emplace_back ( imgui_, false );
	}

	void draw3D () override
	{
		const mat4 p = getDefaultProjection ();
		const mat4 view = camera_.getViewMatrix ();

		multiRenderer_.setMatrices ( p, view );
		multiRenderer2_.setMatrices ( p, view );

		multiRenderer_.setCameraPosition ( positioner_.getPosition () );
		multiRenderer2_.setCameraPosition ( positioner_.getPosition () );
	}
};

int main ()
{
	MyApp app;
	app.mainLoop ();
	return 0;
}