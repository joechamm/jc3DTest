#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkFramework/LineCanvas.h>
#include <jcVkFramework/vkFramework/QuadRenderer.h>
#include <jcVkFramework/vkFramework/GuiRenderer.h>
#include <jcVkFramework/vkFramework/VulkanShaderProcessor.h>
#include <jcVkFramework/vkFramework/InfinitePlaneRenderer.h>

#include <jcCommonFramework/ResourceString.h>

bool g_RotateModel = true;

float g_ModelAngle = 0.0f;

float g_LightAngle = 60.0f;
float g_LightNear = 1.0f;
float g_LightFar = 100.0f;

float g_LightDist = 50.0f;
float g_LightXAngle = -0.5f;
float g_LightYAngle = 0.55f;

std::string g_meshFile = appendToRoot ( "assets/models/rubber_duck/scene.gltf" );
const float g_meshScale = 20.0f;

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

struct Uniforms
{
	mat4 mvp;
	mat4 model;
	mat4 shadowMatrix;
	vec4 cameraPos;
	vec4 lightPos;
	float meshScale;
	float effects;
};

static_assert( sizeof ( Uniforms ) == sizeof ( float ) * 58 );

struct MyApp : public CameraApp
{
private:
	std::vector<float> meshVertices_;
	std::vector<unsigned int> meshIndices_;

	std::pair<BufferAttachment, BufferAttachment> meshBuffer_;
	std::pair<BufferAttachment, BufferAttachment> planeBuffer_;
	
	VulkanBuffer meshUniformBuffer_;
	VulkanBuffer shadowUniformBuffer_;

	VulkanTexture meshDepth_;
	VulkanTexture meshColor_;
	VulkanTexture meshShadowDepth_;
	VulkanTexture meshShadowColor_;

	OffscreenMeshRenderer meshRenderer_;
	OffscreenMeshRenderer depthRenderer_;
	OffscreenMeshRenderer planeRenderer_;

	LineCanvas canvas_;
	
	QuadRenderer quads_;
	GuiRenderer imgui_;
public:
	MyApp ()
		: CameraApp ( 1600, 900 )
		, meshBuffer_ ( ctx_.resources_.loadMeshToBuffer ( g_meshFile, true, true, meshVertices_, meshIndices_ ) )
		, planeBuffer_ ( ctx_.resources_.createPlaneBuffer_XY ( 2.0f, 2.0f ) )
		, meshUniformBuffer_ ( ctx_.resources_.addUniformBuffer ( sizeof ( Uniforms ) ) )
		, shadowUniformBuffer_ ( ctx_.resources_.addUniformBuffer ( sizeof ( Uniforms ) ) )
		, meshDepth_ ( ctx_.resources_.addDepthTexture () )
		, meshColor_ ( ctx_.resources_.addColorTexture () )
		, meshShadowDepth_ ( ctx_.resources_.addDepthTexture () )
		, meshShadowColor_ ( ctx_.resources_.addColorTexture () )
		, meshRenderer_ ( ctx_, meshUniformBuffer_, meshBuffer_,
			{
				fsTextureAttachment ( meshShadowDepth_ ),
				fsTextureAttachment ( ctx_.resources_.loadTexture2D ( appendToRoot ( "assets/models/rubber_duck/textures/Duck_baseColor.png" ).c_str () ) )
			},
			{ meshColor_, meshDepth_ }, getShaderFilenamesWithRoot ( "assets/shaders/VK08_scene.vert", "assets/shaders/VK08_scene.frag" ) )
		, depthRenderer_ ( ctx_, shadowUniformBuffer_, meshBuffer_, {},
			{ meshShadowColor_, meshShadowDepth_ }, getShaderFilenamesWithRoot ( "assets/shaders/VK08_shadow.vert", "assets/shaders/VK08_shadow.frag" ), true )
		, planeRenderer_ ( ctx_, meshUniformBuffer_, planeBuffer_,
			{
				fsTextureAttachment ( meshShadowDepth_ ),
				fsTextureAttachment ( ctx_.resources_.loadTexture2D ( appendToRoot ( "assets/images/sampleSTB.jpg" ).c_str () ) )
			},
			{ meshColor_, meshDepth_ }, getShaderFilenamesWithRoot ( "assets/shaders/VK08_scene.vert", "assets/shaders/VK08_scene.frag" ) )
		, canvas_ ( ctx_, { meshColor_, meshDepth_ }, true )
		, quads_ ( ctx_, { meshColor_, meshDepth_, meshShadowColor_, meshShadowDepth_ } )
		, imgui_ ( ctx_, { meshShadowColor_, meshShadowDepth_ } )
	{
		onScreenRenderers_.emplace_back ( canvas_ );
		onScreenRenderers_.emplace_back ( meshRenderer_ );
		onScreenRenderers_.emplace_back ( depthRenderer_ );
		onScreenRenderers_.emplace_back ( planeRenderer_ );
		
		onScreenRenderers_.emplace_back ( quads_, false );
		onScreenRenderers_.emplace_back ( imgui_, false );

		positioner_.lookAt ( vec3 ( -85.0f, 85.0f, 85.0f ), vec3 ( 0.0f, 0.0f, 0.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
		
		printf ( "Verts: %d\n", ( int ) meshVertices_.size () / 8 );

		canvas_.clear ();
	}

	void update ( float deltaSeconds ) override
	{
		if ( g_RotateModel )
		{
			g_ModelAngle += deltaSeconds;
		}

		while ( g_ModelAngle > 360.0f )
		{
			g_ModelAngle -= 360.0f;
		}
	}

	void drawUI () override
	{
		ImGui::Begin ( "Control", nullptr );
		ImGui::Checkbox ( "Rotate model", &g_RotateModel );
		ImGui::Text ( "Light parameters", nullptr );
		ImGui::SliderFloat ( "Proj::Angle", &g_LightAngle, 10.0f, 170.0f );
		ImGui::SliderFloat ( "Proj::Near", &g_LightNear, 0.1f, 5.0f );
		ImGui::SliderFloat ( "Proj::Far", &g_LightFar, 0.1f, 100.0f );
		ImGui::SliderFloat ( "Pos::Dist", &g_LightDist, 0.5f, 100.0f );
		ImGui::SliderFloat ( "Pos::AngleX", &g_LightXAngle, -3.15f, +3.15f );
		ImGui::SliderFloat ( "Pos::AngleY", &g_LightYAngle, -3.15f, +3.15f );
		ImGui::End ();

		imguiTextureWindow ( "Color", 1 );
		imguiTextureWindow ( "Depth", 2 | ( 0x1 << 16 ) );
	}

	void draw3D () override
	{
		const mat4 proj = getDefaultProjection ();
		const mat4 view = glm::scale ( mat4 ( 1.0f ), vec3 ( 1.0f, -1.0f, 1.0f ) ) * camera_.getViewMatrix ();

		const mat4 m1 = glm::rotate (
			glm::translate ( mat4 ( 1.0f ), vec3 ( 0.0f, 0.5f, -1.5f ) ) * glm::rotate ( mat4 ( 1.0f ), -0.5f * glm::pi<float> (), vec3 ( 1, 0, 0 ) ),
			g_ModelAngle, vec3 ( 0.0f, 0.0f, 1.0f ) );

		canvas_.setCameraMatrix ( proj * view );

		quads_.clear ();
		quads_.quad ( -1.0f, -1.0f, 1.0f, 1.0f, 0 );
		canvas_.clear ();

		const mat4 rotY = glm::rotate ( mat4 ( 1.0f ), g_LightYAngle, vec3 ( 0.0f, 1.0f, 0.0f ) );
		const mat4 rotX = glm::rotate ( rotY, g_LightXAngle, vec3 ( 1.0f, 0.0f, 0.0f ) );
		const vec4 lightPos = rotX * vec4 ( 0.0f, 0.0f, g_LightDist, 1.0f );

		const mat4 lightProj = glm::perspective ( glm::radians ( g_LightAngle ), 1.0f, g_LightNear, g_LightFar );
		const mat4 lightView = glm::lookAt ( vec3 ( lightPos ), vec3 ( 0.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );

		renderCameraFrustrum ( canvas_, lightView, lightProj, vec4 ( 0.0f, 1.0f, 0.0f, 1.0f ) );
		
		const Uniforms uniDepth = {
			.mvp = lightProj * lightView * m1,
			.model = m1,
			.shadowMatrix = mat4 ( 1.0f ),
			.cameraPos = vec4 ( camera_.getPosition (), 1.0f ),
			.lightPos = lightPos,
			.meshScale = g_meshScale
		};

		uploadBufferData ( ctx_.vkDev_, shadowUniformBuffer_.memory, 0, &uniDepth, sizeof ( uniDepth ) );

		const Uniforms uni = {
			.mvp = proj * view * m1,
			.model = m1,
			.shadowMatrix = lightProj * lightView,
			.cameraPos = vec4 ( camera_.getPosition (), 1.0f ),
			.lightPos = lightPos,
			.meshScale = g_meshScale
		};

		uploadBufferData(ctx_.vkDev_, meshUniformBuffer_.memory, 0, &uni, sizeof(uni));
	}
};

int main ()
{
	MyApp app;
	app.mainLoop ();
	return 0;
}