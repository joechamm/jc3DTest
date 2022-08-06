#include <algorithm>
#include <execution>
#include <fstream>
#include <filesystem>
#include <string>

#include <assimp/cimport.h>
#include <assimp/material.h>
#include <assimp/pbrmaterial.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <rapidjson/include/rapidjson/istreamwrapper.h>
#include <rapidjson/include/rapidjson/document.h>
#include <rapidjson/include/rapidjson/rapidjson.h>

#include <jcCommonframework/scene/VtxData.h>
#include <jcCommonframework/scene/Material.h>
#include <jcCommonframework/scene/Scene.h>
#include <jcCommonframework/scene/MergeUtil.h>

#include <jcCommonframework/ResourceString.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include <meshoptimizer.h>

namespace fs = std::filesystem;

using std::string;

MeshData g_MeshData;

uint32_t g_indexOffset = 0;
uint32_t g_vertexOffset = 0;

const uint32_t g_numElementsToStore = 3 + 3 + 2;  // pos(vec3) + normal(vec3) + uv(vec2)

struct SceneConfig
{
	string	filename;
	string	outputMesh;
	string	outputScene;
	string	outputMaterials;
	string  outputBoxes;
	float	scale;
	bool	calculateLODs;
	bool	mergeInstances;
};

/**
 * @brief convert an Assimp material to our custom material description
 * @param M pointer to the Assimp material
 * @param files list of texture filenames
 * @param opacityMaps list of textures that need to be combined with transparency maps
 * @return the converted material description
*/

MaterialDescription convertAIMaterialToDescription ( const aiMaterial* M, vector<string>& files, vector<string>& opacityMaps )
{
	MaterialDescription D;

	aiColor4D Color;

	// use the Assimp API getter fuctions to extract individual color parameters
	if ( aiGetMaterialColor ( M, AI_MATKEY_COLOR_AMBIENT, &Color ) == AI_SUCCESS )
	{
		D.emissiveColor_ = { Color.r, Color.g, Color.b, Color.a };
		if ( D.emissiveColor_.w > 1.0f ) D.emissiveColor_.w = 1.0f;
	}

	if ( aiGetMaterialColor ( M, AI_MATKEY_COLOR_DIFFUSE, &Color ) == AI_SUCCESS )
	{
		D.albedoColor_ = { Color.r, Color.g, Color.b, Color.a };
		if ( D.albedoColor_.w > 1.0f ) D.albedoColor_.w = 1.0f;
	}

	if ( aiGetMaterialColor ( M, AI_MATKEY_COLOR_EMISSIVE, &Color ) == AI_SUCCESS )
	{
		D.emissiveColor_.x += Color.r;
		D.emissiveColor_.y += Color.g;
		D.emissiveColor_.z += Color.b;
		D.emissiveColor_.w += Color.a;
		if ( D.emissiveColor_.w > 1.0f ) D.albedoColor_.w = 1.0f;  // TODO: find out if this should be emissiveColor instead of albedoColor... possible error
	}

	const float opaquenessThreshold = 0.05f;
	float Opacity = 1.0f;

	if ( aiGetMaterialFloat ( M, AI_MATKEY_OPACITY, &Opacity ) == AI_SUCCESS )
	{
		D.transparencyFactor_ = glm::clamp ( 1.0f - Opacity, 0.0f, 1.0f );
		if ( D.transparencyFactor_ >= 1.0f - opaquenessThreshold ) D.transparencyFactor_ = 0.0f;
	}

	if ( aiGetMaterialColor ( M, AI_MATKEY_COLOR_TRANSPARENT, &Color ) == AI_SUCCESS )
	{
		const float Opacity = std::max ( std::max ( Color.r, Color.g ), Color.b );
		D.transparencyFactor_ = glm::clamp ( Opacity, 0.0f, 1.0f );
		if ( D.transparencyFactor_ >= 1.0f - opaquenessThreshold ) D.transparencyFactor_ = 0.0f;
		D.alphaTest_ = 0.5f;
	}

	float tmp = 1.0f;
	if ( aiGetMaterialFloat ( M, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, &tmp ) == AI_SUCCESS )
	{
		D.metallicFactor_ = tmp;
	}

	if ( aiGetMaterialFloat ( M, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, &tmp ) == AI_SUCCESS )
	{
		D.roughness_ = { tmp, tmp, tmp, tmp };
	}

	aiString Path;
	aiTextureMapping Mapping;
	unsigned int UVIndex = 0;
	float Blend = 1.0f;
	aiTextureOp TextureOp = aiTextureOp_Add;
	aiTextureMapMode TextureMapMode[2] = { aiTextureMapMode_Wrap, aiTextureMapMode_Wrap };
	unsigned int TextureFlags = 0;

	if ( aiGetMaterialTexture ( M, aiTextureType_EMISSIVE, 0, &Path, &Mapping, &UVIndex, &Blend, &TextureOp, TextureMapMode, &TextureFlags ) == AI_SUCCESS )
	{
		D.emissiveMap_ = addUnique ( files, Path.C_Str () );
	}

	if ( aiGetMaterialTexture ( M, aiTextureType_DIFFUSE, 0, &Path, &Mapping, &UVIndex, &Blend, &TextureOp, TextureMapMode, &TextureFlags ) == AI_SUCCESS )
	{
		D.albedoMap_ = addUnique ( files, Path.C_Str () );
		const string albedoMap = string ( Path.C_Str () );
		if ( albedoMap.find ( "grey_30" ) != albedoMap.npos )
		{
			D.flags_ |= sMaterialFlags_Transparent;
		}
	}

	// first try tangent space normal map
	if ( aiGetMaterialTexture ( M, aiTextureType_NORMALS, 0, &Path, &Mapping, &UVIndex, &Blend, &TextureOp, TextureMapMode, &TextureFlags ) == AI_SUCCESS )
	{
		D.normalMap_ = addUnique ( files, Path.C_Str () );
	}
	// then height map
	if ( D.normalMap_ == 0xFFFFFFFF )
	{
		if ( aiGetMaterialTexture ( M, aiTextureType_HEIGHT, 0, &Path, &Mapping, &UVIndex, &Blend, &TextureOp, TextureMapMode, &TextureFlags ) == AI_SUCCESS )
		{
			D.normalMap_ = addUnique ( files, Path.C_Str () );
		}
	}

	if ( aiGetMaterialTexture ( M, aiTextureType_OPACITY, 0, &Path, &Mapping, &UVIndex, &Blend, &TextureOp, TextureMapMode, &TextureFlags ) == AI_SUCCESS )
	{
		D.opacityMap_ = addUnique ( opacityMaps, Path.C_Str () );
		D.alphaTest_ = 0.5f;
	}

	// patch materials
	aiString Name;
	string materialName;
	if ( aiGetMaterialString ( M, AI_MATKEY_NAME, &Name ) == AI_SUCCESS )
	{
		materialName = Name.C_Str ();
	}
	// apply heuristics
	if ( (materialName.find ( "Glass" ) != std::string::npos) || (materialName.find ( "Vespa_Headlight" ) != std::string::npos) )
	{
		D.alphaTest_ = 0.75f;
		D.transparencyFactor_ = 0.1f;
		D.flags_ |= sMaterialFlags_Transparent;
	}
	else if ( materialName.find ( "Bottle" ) != std::string::npos )
	{
		D.alphaTest_ = 0.54f;
		D.transparencyFactor_ = 0.4f;
		D.flags_ |= sMaterialFlags_Transparent;
	}
	else if ( materialName.find ( "Metal" ) != std::string::npos )
	{
		D.metallicFactor_ = 1.0f;
		D.roughness_ = gpuvec4 ( 0.1f, 0.1f, 0.0f, 0.0f );
	}

	return D;
}

void processLods ( vector<uint32_t>& indices, vector<float>& vertices, vector<vector<uint32_t>>& outLods )
{
	size_t verticesCountIn = vertices.size () / 2;
	size_t targetIndicesCount = indices.size ();

	uint8_t LOD = 1;

	printf ( "\n   LOD0: %i indices", int ( indices.size () ));

	outLods.push_back ( indices );

	while ( targetIndicesCount > 1024 && LOD < 8 )
	{
		targetIndicesCount = indices.size () / 2;

		bool sloppy = false;

		size_t numOptIndices = meshopt_simplify (
			indices.data (),
			indices.data (),
			(uint32_t)indices.size (),
			vertices.data (), verticesCountIn,
			sizeof ( float ) * 3,
			targetIndicesCount, 0.02f );

		// cannot simplify further
		if ( static_cast<size_t>(numOptIndices * 1.1f) > indices.size () )
		{
			if ( LOD > 1 )
			{
				// try harder
				numOptIndices = meshopt_simplifySloppy (
					indices.data (),
					indices.data (),
					indices.size (),
					vertices.data (),
					verticesCountIn,
					sizeof ( float ) * 3,
					targetIndicesCount, 0.02f, nullptr );
				sloppy = true;
				if ( numOptIndices == indices.size () ) break;
			}
			else
			{
				break;
			}
		}

		indices.resize ( numOptIndices );

		meshopt_optimizeVertexCache ( indices.data (), indices.data (), indices.size (), verticesCountIn );

		printf ( "\n    LOD%i: %i indices %s", int ( LOD ), int ( numOptIndices ), sloppy ? "[sloppy]" : "" );

		LOD++;

		outLods.push_back ( indices );
	}
}

Mesh convertAIMesh ( const aiMesh* m, const SceneConfig& cfg )
{
	const bool hasTexCoords = m->HasTextureCoords ( 0 );
	const uint32_t streamElementSize = static_cast<uint32_t>(g_numElementsToStore * sizeof ( float ));

	Mesh result = {
		.streamCount = 1,
		.indexOffset = g_indexOffset,
		.vertexOffset = g_vertexOffset,
		.vertexCount = m->mNumVertices,
		.streamOffset = { g_vertexOffset * streamElementSize },
		.streamElementSize = { streamElementSize }
	};

	// Original data for LOD calculation
	vector<float> srcVertices;
	vector<uint32_t> srcIndices;

	vector<vector<uint32_t>> outLods;

	auto& vertices = g_MeshData.vertexData_;

	for ( size_t i = 0; i != m->mNumVertices; i++ )
	{
		const aiVector3D v = m->mVertices[i];
		const aiVector3D n = m->mNormals[i];
		const aiVector3D t = hasTexCoords ? m->mTextureCoords[0][i] : aiVector3D ();

		if ( cfg.calculateLODs )
		{
			srcVertices.push_back ( v.x );
			srcVertices.push_back ( v.y );
			srcVertices.push_back ( v.z );
		}

		vertices.push_back ( v.x * cfg.scale );
		vertices.push_back ( v.y * cfg.scale );
		vertices.push_back ( v.z * cfg.scale );

		vertices.push_back ( t.x );
		vertices.push_back ( 1.0f - t.y );

		vertices.push_back ( n.x );
		vertices.push_back ( n.y );
		vertices.push_back ( n.z );
	}

	for ( size_t i = 0; i != m->mNumFaces; i++ )
	{
		if ( m->mFaces[i].mNumIndices != 3 ) continue;		
		for ( unsigned j = 0; j != m->mFaces[i].mNumIndices; j++ )
		{
			srcIndices.push_back ( m->mFaces[i].mIndices[j] );
		}
	}

	if ( !cfg.calculateLODs )
	{
		outLods.push_back ( srcIndices );
	}
	else
	{
		processLods ( srcIndices, srcVertices, outLods );
	}

#ifdef _DEBUG
	printf ( "\nCalculated LOD count: %u\n", (unsigned)outLods.size () );
#endif // _DEBUG
	
	uint32_t numIndices = 0;

	for ( size_t l = 0; l < outLods.size (); l++ )
	{
		for ( size_t i = 0; i < outLods[l].size (); i++ )
		{
			g_MeshData.indexData_.push_back ( outLods[l][i] );
		}

		result.lodOffset[l] = numIndices;
		numIndices += (int)outLods[l].size ();
	}

	result.lodOffset[outLods.size ()] = numIndices;
	result.lodCount = (uint32_t)outLods.size ();

	g_indexOffset += numIndices;
	g_vertexOffset += m->mNumVertices;

	return result;
}

void makePrefix ( int ofs )
{
	for ( int i = 0; i < ofs; i++ ) printf ( "\t" );
}

void printMat4 ( const aiMatrix4x4& m )
{
	if ( !m.IsIdentity () )
	{
		for ( int i = 0; i < 4; i++ )
		{
			for ( int j = 0; j < 4; j++ )
			{
				printf ( "%f ;", m[i][j] );
			}
		}
	}
	else
	{
		printf ( " Identity" );
	}
}

mat4 toMat4 ( const aiMatrix4x4& from )
{
	mat4 to;
	to[0][0] = (float)from.a1; 
	to[0][1] = (float)from.b1;
	to[0][2] = (float)from.c1;
	to[0][3] = (float)from.d1;

	to[1][0] = (float)from.a2;
	to[1][1] = (float)from.b2;
	to[1][2] = (float)from.c2;
	to[1][3] = (float)from.d2;

	to[2][0] = (float)from.a3;
	to[2][1] = (float)from.b3;
	to[2][2] = (float)from.c3;
	to[2][3] = (float)from.d3;

	to[3][0] = (float)from.a4;
	to[3][1] = (float)from.b4;
	to[3][2] = (float)from.c4;
	to[3][3] = (float)from.d4;

	return to;
}

void traverse ( const aiScene* sourceScene, Scene& scene, aiNode* node, int parent, int atLevel )
{
	int newNodeID = addNode ( scene, parent, atLevel );
	if ( node->mName.C_Str () )
	{
		makePrefix ( atLevel );
		printf ( "Node[%d].name = %s\n", newNodeID, node->mName.C_Str () );
		
		uint32_t stringID = (uint32_t)scene.names_.size ();
		scene.names_.push_back ( string ( node->mName.C_Str () ) );
		scene.nameForNode_[newNodeID] = stringID;
	}

	for ( size_t i = 0; i < node->mNumMeshes; i++ )
	{
		int newSubNodeID = addNode ( scene, newNodeID, atLevel + 1 );
		uint32_t stringID = (uint32_t)scene.names_.size ();
		scene.names_.push_back ( string ( node->mName.C_Str () ) + "_Mesh_" + std::to_string ( i ) );
		scene.nameForNode_[newSubNodeID] = stringID;

		int mesh = (int)node->mMeshes[i];
		scene.meshes_[newSubNodeID] = mesh;
		scene.materialForNode_[newSubNodeID] = sourceScene->mMeshes[mesh]->mMaterialIndex;

		makePrefix ( atLevel );
		printf ( "Node[%d].SubNode[%d].mesh  =  %d\n", newNodeID, newSubNodeID, (int)mesh );
		makePrefix ( atLevel );
		printf ( "Node[%d].SubNode[%d].material = %d\n", newNodeID, newSubNodeID, sourceScene->mMeshes[mesh]->mMaterialIndex );

		scene.globalTransforms_[newSubNodeID] = mat4 ( 1.0f );
		scene.localTransforms_[newSubNodeID] = mat4 ( 1.0f );		
	}

	scene.globalTransforms_[newNodeID] = mat4 ( 1.0f );
	scene.localTransforms_[newNodeID] = toMat4 ( node->mTransformation );

	if ( node->mParent != nullptr )
	{
		makePrefix ( atLevel );
		printf ( "\tNode[%d].parent       = %s\n", newNodeID, node->mParent->mName.C_Str () );
		makePrefix ( atLevel );
		printf ( "\tNode[%d].localTransform = ", newNodeID );
		printMat4 ( node->mTransformation );
		printf ( "\n" );
	}

	for ( unsigned int n = 0; n < node->mNumChildren; n++ )
	{
		traverse ( sourceScene, scene, node->mChildren[n], newNodeID, atLevel + 1 );
	}
}

void dumpMaterial ( const vector<string>& files, const MaterialDescription& d )
{
	printf("files: %d\n", (int)files.size());
	printf ( "maps: %u%u%u%u%u\n", (uint32_t)d.albedoMap_, (uint32_t)d.ambientOcclusionMap_, (uint32_t)d.emissiveMap_, (uint32_t)d.opacityMap_, (uint32_t)d.metallicRoughnessMap_ );
	printf ( " albedo:	%s\n", (d.albedoMap_ < 0xFFFF) ? files[d.albedoMap_].c_str () : "" );
	printf ( " occlusion:	%s\n", (d.ambientOcclusionMap_ < 0xFFFF) ? files[d.ambientOcclusionMap_].c_str () : "" );
	printf ( " emission:	%s\n", (d.emissiveMap_ < 0xFFFF) ? files[d.emissiveMap_].c_str () : "" );
	printf ( " opacity:	%s\n", (d.opacityMap_ < 0xFFFF) ? files[d.opacityMap_].c_str () : "" );
	printf ( " MeR:	%s\n", (d.metallicRoughnessMap_ < 0xFFFF) ? files[d.metallicRoughnessMap_].c_str () : "" );
	printf ( " Normal:	%s\n", (d.normalMap_ < 0xFFFF) ? files[d.normalMap_].c_str () : "" );
}

string replaceAll ( const string& str, const string& oldSubStr, const string& newSubStr )
{
	string result = str;

	for ( size_t p = result.find ( oldSubStr ); p != std::string::npos; p = result.find ( oldSubStr ) )
	{
		result.replace ( p, oldSubStr.length (), newSubStr );
	}

	return result;
}

/* Convert 8-bit ASCII string to upper case */
string lowercaseString ( const string& s )
{
	string out ( s.length (), ' ' );
	std::transform ( s.begin (), s.end (), out.begin (), tolower );
	return out;
}

/* find a file in directory which "almost" coincides with the origFile (their lowercase versions coincide) */
string findSubstitute ( const string& origFile )
{
	// Make absolute path
	auto apath = fs::absolute ( fs::path ( origFile ) );
	// Absolute lowercase filename [we compare with it]
	auto afile = lowercaseString ( apath.filename ().string () );
	// Directory where in which the file should be
	auto dir = fs::path ( origFile ).remove_filename ();

	// Iterate each file non-recursively and compare lowercase absolute path with 'afile'
	for ( auto& p : fs::directory_iterator ( dir ) )
	{
		if ( afile == lowercaseString ( p.path ().filename ().string () ) )
		{
			return p.path ().string ();
		}
	}

	return string{};
}

string fixTextureFile ( const string& file )
{
	// TODO: check the findSubstitute() function
	return fs::exists ( file ) ? file : findSubstitute ( file );
}

string convertTexture ( const string& file, const string& basePath, unordered_map<string, uint32_t>& opacityMapIndices, const vector<string>& opacityMaps )
{
	const int maxNewWidth = 512;
	const int maxNewHeight = 512;

	const auto srcFile = replaceAll ( basePath + file, "\\", "/" );
	const auto newFile = appendToRoot ( "assets/out_textures/" ) + lowercaseString ( replaceAll ( replaceAll ( srcFile, "..", "__" ), "/", "__" ) + string ( "__rescaled" ) ) + string ( ".png" );

	// load this image
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load ( fixTextureFile ( srcFile ).c_str (), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
	uint8_t* src = pixels;
	texChannels = STBI_rgb_alpha;

	vector<uint8_t> tmpImage ( maxNewWidth * maxNewHeight * 4 );

	if ( !src )
	{
		printf ( "Failed to load [%s] texture\n", srcFile.c_str () );
		texWidth = maxNewWidth;
		texHeight = maxNewHeight;
		texChannels = STBI_rgb_alpha;
		src = tmpImage.data ();
	}
	else
	{
		printf ( "Loaded [%s] %d%d texture with %d channels\n", srcFile.c_str (), texWidth, texHeight, texChannels );
	}

	if ( opacityMapIndices.count ( file ) > 0 )
	{
		const auto opacityMapFile = replaceAll ( basePath + opacityMaps[opacityMapIndices[file]], "\\", "/" );
		int opacityWidth, opacityHeight;
		stbi_uc* opacityPixels = stbi_load ( fixTextureFile ( opacityMapFile ).c_str (), &opacityWidth, &opacityHeight, nullptr, 1 );

		if ( !opacityPixels )
		{
			printf ( "Failed to load opacity mask [%s]\n", opacityMapFile.c_str () );
		}

		assert ( opacityPixels );
		assert ( texWidth == opacityWidth );
		assert ( texHeight == opacityHeight );

		// store the opacity mask in the alpha component of this image
		if ( opacityPixels )
		{
			for ( int y = 0; y != opacityHeight; y++ )
			{
				for ( int x = 0; x != opacityWidth; x++ )
				{
					src[(y * opacityWidth + x) * texChannels + 3] = opacityPixels[y * opacityWidth + x];
				}
			}
		}

		stbi_image_free ( opacityPixels );
	}

	const uint32_t imgSize = texWidth * texHeight * texChannels;
	vector<uint8_t> mipData ( imgSize );
	uint8_t* dst = mipData.data ();

	const int newW = std::min ( texWidth, maxNewWidth );
	const int newH = std::min ( texHeight, maxNewHeight );

	stbir_resize_uint8 ( src, texWidth, texHeight, 0, dst, newW, newH, 0, texChannels );

	stbi_write_png ( newFile.c_str (), newW, newH, texChannels, dst, 0 );

	if ( pixels )
	{
		stbi_image_free ( pixels );
	}

	return newFile;
}

void convertAndDownscaleAllTextures ( const vector<MaterialDescription>& materials, const string& basePath, vector<string>& files, vector<string>& opacityMaps )
{
	unordered_map<string, uint32_t> opacityMapIndices ( files.size () );

	for ( const auto& m : materials )
	{
		if ( m.opacityMap_ != 0xFFFFFFFF && m.albedoMap_ != 0xFFFFFFFF )
		{
			opacityMapIndices[files[m.albedoMap_]] = (uint32_t)m.opacityMap_;
		}
	}

	auto converter = [&]( const string& s ) -> string
	{
		return convertTexture ( s, basePath, opacityMapIndices, opacityMaps );
	};

	std::transform ( std::execution::par, std::begin ( files ), std::end ( files ), std::begin ( files ), converter );
}

// TODO: use absolute filepaths
vector<SceneConfig> readConfigFile ( const char* cfgFilename )
{
	std::ifstream ifs ( cfgFilename );
	if ( !ifs.is_open() )
	{
		printf ( "Failed to load configuration file.\n" );
		exit ( EXIT_FAILURE );
	}

	rapidjson::IStreamWrapper isw ( ifs );
	rapidjson::Document document;
	const rapidjson::ParseResult parseResult = document.ParseStream ( isw );
	assert ( !parseResult.IsError () );
	assert ( document.IsArray () );

	vector<SceneConfig> configList;

	for ( rapidjson::SizeType i = 0; i < document.Size (); i++ )
	{
		configList.emplace_back ( SceneConfig{
			.filename = document[i]["input_scene"].GetString (),
			.outputMesh = document[i]["output_mesh"].GetString (),
			.outputScene = document[i]["output_scene"].GetString (),
			.outputMaterials = document[i]["output_materials"].GetString (),
			.outputBoxes = document[i].HasMember("output_boxes") ? document[i]["output_boxes"].GetString() : string(),
			.scale = (float)document[i]["scale"].GetDouble (),
			.calculateLODs = document[i]["calculate_LODs"].GetBool (),
			.mergeInstances = document[i]["merge_instances"].GetBool ()
			} );
	}

	return configList;
}

vector<SceneConfig> readConfigFileAppendToRoot ( const char* cfgFilename )
{
	std::ifstream ifs ( cfgFilename );
	if ( !ifs.is_open () )
	{
		printf( "Failed to load configuration file.\n" );
		exit ( EXIT_FAILURE );
	}

	rapidjson::IStreamWrapper isw ( ifs );
	rapidjson::Document document;
	const rapidjson::ParseResult parseResult = document.ParseStream ( isw );
	assert ( !parseResult.IsError () );
	assert ( document.IsArray () );

	vector<SceneConfig> configList;

	for ( rapidjson::SizeType i = 0; i < document.Size (); i++ )
	{
		string inSceneFilename = appendToRoot ( document[i]["input_scene"].GetString () );
		string outMeshFilename = appendToRoot ( document[i]["output_mesh"].GetString () );
		string outSceneFilename = appendToRoot ( document[i]["output_scene"].GetString () );
		string outMaterialsFilename = appendToRoot ( document[i]["output_materials"].GetString () );
		string outBoxesFilename = document[i].HasMember("output_boxes") ? appendToRoot(document[i]["output_boxes"].GetString()) : string();
		float scale = (float)document[i]["scale"].GetDouble ();
		bool calculateLODs = document[i]["calculate_LODs"].GetBool ();
		bool mergeInstances = document[i]["merge_instances"].GetBool ();
		configList.push_back ( SceneConfig{
			.filename = inSceneFilename,
			.outputMesh = outMeshFilename,
			.outputScene = outSceneFilename,
			.outputMaterials = outMaterialsFilename,
			.outputBoxes = outBoxesFilename,
			.scale = scale,
			.calculateLODs = calculateLODs,
			.mergeInstances = mergeInstances
			} 
		);
	}

	return configList;
}

void processScene ( const SceneConfig& cfg )
{
	// clear mesh data from previous scene
	g_MeshData.meshes_.clear ();
	g_MeshData.boxes_.clear ();
	g_MeshData.indexData_.clear ();
	g_MeshData.vertexData_.clear ();

	g_indexOffset = 0;
	g_vertexOffset = 0;

	// extract base model path
	const std::size_t pathSeparator = cfg.filename.find_last_of ( "/\\" );
	const string basePath = (pathSeparator != std::string::npos) ? cfg.filename.substr ( 0, pathSeparator + 1 ) : std::string ();

	const unsigned int flags = 0 |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_LimitBoneWeights |
		aiProcess_SplitLargeMeshes |
		aiProcess_ImproveCacheLocality |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_FindDegenerates |
		aiProcess_FindInvalidData |
		aiProcess_GenUVCoords;

	printf ( "Loading scene from '%s'...\n", cfg.filename.c_str () );

	const aiScene* scene = aiImportFile ( cfg.filename.c_str (), flags );

	if ( !scene || !scene->HasMeshes () )
	{
		printf ( "Unable to load '%s'\n", cfg.filename.c_str () );
		exit ( EXIT_FAILURE );
	}

	// 1. Mesh conversion as in Chapter 5
	g_MeshData.meshes_.reserve ( scene->mNumMeshes );
	g_MeshData.boxes_.reserve ( scene->mNumMeshes );

	for ( unsigned int i = 0; i != scene->mNumMeshes; i++ )
	{
		printf ( "\nConverting meshes %u/%u...", i + 1, scene->mNumMeshes );
		Mesh mesh = convertAIMesh ( scene->mMeshes[i], cfg );
	}
}

void processSceneAndSave ( const SceneConfig& cfg )
{
	// clear mesh data from previous scene
	g_MeshData.meshes_.clear ();
	g_MeshData.boxes_.clear ();
	g_MeshData.indexData_.clear ();
	g_MeshData.vertexData_.clear ();

	g_indexOffset = 0;
	g_vertexOffset = 0;

	// extract base model path
	const std::size_t pathSeparator = cfg.filename.find_last_of ( "/\\" );
	const string basePath = (pathSeparator != std::string::npos) ? cfg.filename.substr ( 0, pathSeparator + 1 ) : std::string ();

	const unsigned int flags = 0 |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_LimitBoneWeights |
		aiProcess_SplitLargeMeshes |
		aiProcess_ImproveCacheLocality |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_FindDegenerates |
		aiProcess_FindInvalidData |
		aiProcess_GenUVCoords;

	printf ( "Loading scene from '%s'...\n", cfg.filename.c_str () );

	const aiScene* scene = aiImportFile ( cfg.filename.c_str (), flags );

	if ( !scene || !scene->HasMeshes () )
	{
		printf ( "Unable to load '%s'\n", cfg.filename.c_str () );
		exit ( EXIT_FAILURE );
	}

	// 1. Mesh conversion as in Chapter 5
	g_MeshData.meshes_.reserve ( scene->mNumMeshes );
	g_MeshData.boxes_.reserve ( scene->mNumMeshes );

	for ( unsigned int i = 0; i != scene->mNumMeshes; i++ )
	{
#ifdef _DEBUG
		printf ( "\nConverting meshes %u/%u...", i + 1, scene->mNumMeshes );
#endif
		Mesh mesh = convertAIMesh ( scene->mMeshes[i], cfg );
		g_MeshData.meshes_.push_back ( mesh );
	}

	recalculateBoundingBoxes ( g_MeshData );
	
	// 2. Save meshes to file 
	saveMeshData ( cfg.outputMesh.c_str (), g_MeshData );

	Scene outScene;

	// 3. Material conversion 
	std::vector<MaterialDescription> materials;
	std::vector<std::string>& materialNames = outScene.materialNames_;

	std::vector<std::string> files;
	std::vector<std::string> opacityMaps;

	for ( unsigned int m = 0; m < scene->mNumMaterials; m++ )
	{
		aiMaterial* mMat = scene->mMaterials[m];
		
		printf ( "Material [%s] %u\n", mMat->GetName ().C_Str (), m );
		materialNames.push_back ( std::string ( mMat->GetName ().C_Str () ) );
		
		MaterialDescription mMatDesc = convertAIMaterialToDescription ( mMat, files, opacityMaps );
		materials.push_back ( mMatDesc );
#ifdef _DEBUG
		dumpMaterial ( files, mMatDesc );
#endif // _DEBUG
	}

	// 4. Texture processing, rescaling and packing
	convertAndDownscaleAllTextures ( materials, basePath, files, opacityMaps );

	// 5. Save materials to file
	saveMaterials ( cfg.outputMaterials.c_str (), materials, files );

	// 6. Scene hierarchy conversion 
	traverse ( scene, outScene, scene->mRootNode, -1, 0 );

	// 7. Save scene to file
	saveScene ( cfg.outputScene.c_str (), outScene );
	
}

/* Chapter 9: Merge meshes (interior/exterior) */
void mergeBistro ()
{
	Scene scene1, scene2;
	vector<Scene*> scenes = { &scene1, &scene2 };

	MeshData m1, m2;
	MeshFileHeader header1 = loadMeshData ( appendToRoot ( "assets/meshes/test.meshes" ).c_str (), m1 );
	MeshFileHeader header2 = loadMeshData ( appendToRoot ( "assets/meshes/test2.meshes" ).c_str (), m2 );

	vector<uint32_t> meshCounts = { header1.meshCount, header2.meshCount };

	loadScene ( appendToRoot ( "assets/meshes/test.scene" ).c_str (), scene1 );
	loadScene ( appendToRoot ( "assets/meshes/test2.scene" ).c_str (), scene2 );

	Scene scene;
	mergeScenes ( scene, scenes, {}, meshCounts );
	
	MeshData meshData;
	vector<MeshData*> meshDatas = { &m1, &m2 };

	MeshFileHeader header = mergeMeshData ( meshData, meshDatas );

	// now the material lists
	vector<MaterialDescription> materials1, materials2;
	vector<string> textureFiles1, textureFiles2;
	loadMaterials ( appendToRoot ( "assets/meshes/test.materials").c_str(), materials1, textureFiles1 );
	loadMaterials ( appendToRoot ( "assets/meshes/test2.materials" ).c_str (), materials2, textureFiles2 );

	vector<MaterialDescription> allMaterials;
	vector<string> allTextures;

	mergeMaterialLists ( { &materials1, &materials2 },
		{ &textureFiles1, &textureFiles2 },
		allMaterials, allTextures );

	saveMaterials ( appendToRoot ( "assets/meshes/bistro_all.materials").c_str(), allMaterials, allTextures );

	printf ( "[Unmerged] scene items: %d\n", (int)scene.hierarchy_.size () );
	mergeScene ( scene, meshData, "Foliage_Linde_Tree_Large_Orange_Leaves" );
	printf ( "[Merged orange leaves] scene items: %d\n", (int)scene.hierarchy_.size () );
	mergeScene ( scene, meshData, "Foliage_Linde_Tree_Large_Green_Leaves" );
	printf ( "[Merged green leaves] scene items: %d\n", (int)scene.hierarchy_.size () );
	mergeScene ( scene, meshData, "Foliage_Linde_Tree_Large_Trunk" );
	printf ( "[Merged trunk] scene items: %d\n", (int)scene.hierarchy_.size () );

	recalculateBoundingBoxes ( meshData );

	saveMeshData ( appendToRoot ( "assets/meshes/bistro_all.meshes").c_str(), meshData );
	saveScene ( appendToRoot ( "assets/meshes/bistro_all.scene" ).c_str (), scene );
}

int main ()
{
	fs::create_directory ( appendToRoot ( "assets/out_textures" ).c_str());
//	const auto configs = readConfigFile ( appendToRoot ( "assets/sceneconverter.json" ).c_str());
	const auto configs = readConfigFileAppendToRoot ( appendToRoot ( "assets/sceneconverter.json" ).c_str () );
	for ( const auto& cfg : configs )
	{
#ifdef _DEBUG
		printf ( "Converting '%s'\n", cfg.filename.c_str () );
		printf ( "OutputScene: %s\nOutputMesh: %s\nOutputMaterials: %s\n", cfg.outputScene.c_str (), cfg.outputMesh.c_str (), cfg.outputMaterials.c_str () );
#endif
		
		processSceneAndSave ( cfg );
	}

	// Final step: optimize bistro scene
	return 0;
}