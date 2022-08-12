#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkFramework/GuiRenderer.h>
#include <jcVkFramework/vkFramework/MultiRenderer.h>
#include <jcVkFramework/vkFramework/InfinitePlaneRenderer.h>

#include <jcCommonFramework/ResourceString.h>

#include "Physics.h"

#include <random>

constexpr int maxCubes = 100;

constexpr double maxCubeSize = 5.0;
constexpr double minCubeSize = -5.0;

constexpr double maxHeight = 300.0;
constexpr double minHeight = 2.0;

constexpr double minXCoord = -10.0;
constexpr double maxXCoord = 10.0;

constexpr double maxZCoord = 15.0;
constexpr double minZCoord = 2.0;

constexpr double minMass = 0.1;
constexpr double maxMass = 100.0;

constexpr double minQuat = -1.0;
constexpr double maxQuat = 1.0;

std::random_device rd;
std::mt19937 gen;
std::uniform_real_distribution<> sizeDist ( minCubeSize, maxCubeSize );
std::uniform_real_distribution<> heightDist ( minHeight, maxHeight );
std::uniform_real_distribution<> xDist ( minXCoord, maxXCoord );
std::uniform_real_distribution<> zDist ( minZCoord, maxZCoord );
std::uniform_real_distribution<> massDist ( minMass, maxMass );
std::uniform_real_distribution<> quatDist ( minQuat, maxQuat );

vec3 getRandomSize (std::mt19937& gen )
{
	return vec3(sizeDist(gen ), sizeDist ( gen ), sizeDist ( gen ) );
}

vec3 getRandomPosition (std::mt19937& gen )
{
	return vec3(xDist(gen ), heightDist ( gen ), zDist ( gen ) );
}

btQuaternion getRandomOrientation ( std::mt19937& gen )
{
	return btQuaternion ( quatDist ( gen ), quatDist ( gen ), quatDist ( gen ), 1.0 ).normalized ();
}

float getRandomMass ( std::mt19937& gen )
{
	return massDist ( gen );
}

struct MyApp : public CameraApp
{
private:
	InfinitePlaneRenderer plane_;
	
	VKSceneData sceneData_;
	MultiRenderer multiRenderer_;

	GuiRenderer imgui_;

	Physics physics_;

	//std::random_device rd_;
	//std::mt19937 eng_;
	//std::uniform_real_distribution<> sizeDist_;
	//std::uniform_real_distribution<> heightDist_;
	//std::uniform_real_distribution<> xCoordDist_;
	//std::uniform_real_distribution<> zCoordDist_;
	//std::uniform_real_distribution<> massDist_;
	//std::uniform_real_distribution<> quatDist_;

public:
	MyApp ()
		: CameraApp ( 1600, 900 )
		, plane_ ( ctx_ )
		, sceneData_ ( ctx_, appendToRoot ( "assets/meshes/cube.meshes" ).c_str (), appendToRoot ( "assets/meshes/cube.scene" ).c_str (), appendToRoot ( "assets/meshes/cube.material" ).c_str (), {}, {} )
		, multiRenderer_ ( ctx_, sceneData_, appendToRoot ( "assets/shaders/VK09_Simple.vert" ).c_str (), appendToRoot ( "assets/shaders/VK09_Simple.frag" ).c_str () )
		, imgui_ ( ctx_ )
//		, eng_(rd_())
//		, sizeDist_(minCubeSize, maxCubeSize)
//		, heightDist_(minHeight, maxHeight)
//		, xCoordDist_(minXCoord, maxXCoord)
//		, zCoordDist_(minZCoord, maxZCoord)
//		, massDist_(minMass, maxMass)
//		, quatDist_(minQuat, maxQuat)
	{
		
		positioner_ = CameraPositioner_FirstPerson ( vec3 ( 0.0f, 50.0f, 100.0f ), vec3 ( 0.0f, 0.0f, 0.0f ), vec3 ( 0.0f, -1.0f, 0.0f ) );

		onScreenRenderers_.emplace_back ( plane_, false );
		onScreenRenderers_.emplace_back ( multiRenderer_ );
		onScreenRenderers_.emplace_back ( imgui_, false );		
		
		for ( int i = 0; i < maxCubes; i++ )
		{
			physics_.addBox ( vec3 ( 1 ), btQuaternion ( 0, 0, 0, 1 ), vec3 ( 0.0f, 2.0f + 3.0f * i, 0.0f ), 3.0f );
		}
	}

	//inline vec3 getRandomHalfSize () const
	//{
	//	return vec3 ( sizeDist_ ( eng_ ), sizeDist_ ( eng_ ), sizeDist_ ( eng_ ) );
	//}

	//inline vec3 getRandomPosition () const
	//{
	//	return vec3 ( xCoordDist_ ( eng_ ), heightDist_ ( eng_ ), zCoordDist_ ( eng_ ) );
	//}

	//inline btQuaternion getRandomOrientation () const
	//{
	//	btQuaternion orientation ( quatDist_ ( eng_ ), quatDist_ ( eng_ ), quatDist_ ( eng_ ), 1.0f );
	//	orientation.normalize ();
	//	return orientation;
	//}

	//inline float getRandomMass () const
	//{
	//	return massDist_ ( eng_ );
	//}

	void draw3D () override
	{
		const mat4 p = getDefaultProjection ();
		const mat4 view = camera_.getViewMatrix ();

		multiRenderer_.setMatrices ( p, view );
		multiRenderer_.setCameraPosition ( positioner_.getPosition () );

		plane_.setMatrices ( p, view, mat4 ( 1.0f ) );

		sceneData_.scene_.globalTransforms_ [ 0 ] = mat4 ( 1.0f );
		for ( size_t i = 1; i < physics_.boxTransform_.size (); i++ )
		{
			sceneData_.scene_.globalTransforms_ [ i ] = physics_.boxTransform_ [ i ];
		}
	}

	void drawUI () override
	{
		ImGui::Begin ( "Settings", nullptr );
		ImGui::Text ( "FPS: %.2f", getFPS () );
		if ( ImGui::Button ( "Add body" ) )
		{
			if ( physics_.boxTransform_.size () < maxCubes )
			{
				vec3 halfSize = getRandomSize ( gen );
				vec3 position = getRandomPosition ( gen );
				btQuaternion orientation = getRandomOrientation ( gen );
				float mass = getRandomMass ( gen );
				physics_.addBox ( halfSize, orientation, position, mass );						
			}
		}
		ImGui::End ();
	}

	void update ( float deltaSeconds ) override
	{
		CameraApp::update ( deltaSeconds );

		physics_.update ( deltaSeconds );

		sceneData_.uploadGlobalTransforms ();
	}
};

void generateMeshFile ()
{
	constexpr uint32_t cubeVtxCount = 8;
	constexpr uint32_t cubeIdxCount = 36;

	Mesh cubeMesh { .lodCount = 1, .streamCount = 1, .lodOffset = { 0, cubeIdxCount }, .streamOffset = { 0 } };

	MeshData md = {
		.indexData_ = std::vector<uint32_t> ( cubeIdxCount, 0 ),
		.vertexData_ = std::vector<float> ( cubeVtxCount * 8, 0 ),
		.meshes_ = std::vector<Mesh>{cubeMesh},
		.boxes_ = std::vector<BoundingBox> ( 1 )
	};

	saveMeshData ( appendToRoot ( "assets/meshes/cube.meshes" ).c_str (), md );
}

void generateData ()
{
	const int numCubes = 100;

	Scene cubeScene;
	addNode ( cubeScene, -1, 0 );
	for ( int i = 0; i < numCubes; i++ )
	{
		addNode ( cubeScene, 0, 1 );
		cubeScene.meshes_ [ i + 1 ] = 0;
		cubeScene.materialForNode_ [ i + 1 ] = 0;
	}

	saveScene ( appendToRoot ( "assets/meshes/cube.scene" ).c_str (), cubeScene );

	std::vector<std::string> files = { appendToRoot ( "assets/images/sampleSTB.jpg" ) };
	std::vector<MaterialDescription> materials = { {.albedoColor_ = gpuvec4 ( 1, 0, 0, 1 ), .albedoMap_ = 0xFFFFFFF, .normalMap_ = 0xFFFFFFF } };
	saveMaterials( appendToRoot ( "assets/meshes/cube.material" ).c_str (), materials, files );
}

int main (int argc, const char** argv)
{
	gen = std::mt19937 ( rd () );

	generateMeshFile ();
	generateData ();

	MyApp app;
	app.mainLoop ();
	return 0;
}