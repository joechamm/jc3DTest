#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include "VtxData.h"

#include <meshoptimizer.h>

MeshData g_meshData;

uint32_t g_indexOffset = 0;
uint32_t g_vertexOffset = 0;

//uint32_t g_numElementsToStore = 3;
constexpr uint32_t g_numElementsToStore = 3 + 3 + 2;

float g_meshScale = 0.01f;
bool g_calculateLODs = false;

#ifndef NDEBUG
bool verbose = true;
#else
bool verbose = false;
#endif 

bool g_exportTextures = false;
bool g_exportNormals = false;

void processLods ( std::vector<uint32_t>& indices, std::vector<float>& vertices, std::vector<std::vector<uint32_t>>& outLods )
{
	size_t verticesCountIn = vertices.size () / 3;
	size_t targetIndicesCount = indices.size ();

	uint8_t LOD = 1;

	printf ( "\n  LOD0: %i indices", int ( indices.size () ));

	outLods.push_back ( indices );

	while ( targetIndicesCount > 1024 && LOD < 8 )
	{
		targetIndicesCount = indices.size () / 2;

		bool sloppy = false;

		size_t numOptIndices = meshopt_simplify (
			indices.data (),
			indices.data (), static_cast<uint32_t>(indices.size ()),
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
					indices.data (), indices.size (),
					vertices.data (), verticesCountIn,
					sizeof ( float ) * 3,
					targetIndicesCount, 0.02f );
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

		printf ( "\n  LOD%i: %i indices %s", int ( LOD ), int ( numOptIndices ), sloppy ? "[sloppy]" : "" );

		LOD++;

		outLods.push_back ( indices );
	}
}

Mesh convertAIMesh ( const aiMesh* m )
{
	/* Check if there is a set of texture coordinates present */
	const bool hasTexCoords = m->HasTextureCoords ( 0 );

	/* The size of the stream element in bytes is directly calculated from the number of elements per vertex. Since we are storing each component as a floating point value, no branching logic is required now. */
	const uint32_t streamElementSize = static_cast<uint32_t>(g_numElementsToStore * sizeof ( float ));

	// Original data for LOD calculation
	std::vector<float> srcVertices;
	std::vector<uint32_t> srcIndices;

	std::vector<std::vector<uint32_t>> outLods;

	auto& vertices = g_meshData.vertexData_;

	for ( size_t i = 0; i != m->mNumVertices; i++ )
	{
		const aiVector3D v = m->mVertices[i];
		const aiVector3D n = m->mNormals[i];
		const aiVector3D t = hasTexCoords ? m->mTextureCoords[0][i] : aiVector3D ();

		if ( g_calculateLODs )
		{
			srcVertices.push_back ( v.x );
			srcVertices.push_back ( v.y );
			srcVertices.push_back ( v.z );
		}

		vertices.push_back ( v.x * g_meshScale );
		vertices.push_back ( v.y * g_meshScale );
		vertices.push_back ( v.z * g_meshScale );

		/*
		if ( g_exportTextures )
		{
			vertices.push_back ( t.x );
			vertices.push_back ( 1.0f - t.y );
		}

		if ( g_exportNormals )
		{
			vertices.push_back ( n.x );
			vertices.push_back ( n.y );
			vertices.push_back ( n.z );
		}
		*/

		vertices.push_back ( t.x );
		vertices.push_back ( 1.0f - t.y );
		vertices.push_back ( n.x );
		vertices.push_back ( n.y );
		vertices.push_back ( n.z );
	}

	Mesh result = {
		.streamCount = 1,
		.indexOffset = g_indexOffset,
		.vertexOffset = g_vertexOffset,
		.vertexCount = m->mNumVertices,
		.streamOffset = { g_vertexOffset * streamElementSize },
		.streamElementSize = { streamElementSize }
	};

	for ( size_t i = 0; i != m->mNumFaces; i++ )
	{
		if ( m->mFaces[i].mNumIndices != 3 )
		{
			continue;
		}
		for ( unsigned j = 0; j != m->mFaces[i].mNumIndices; j++ )
		{
			srcIndices.push_back ( m->mFaces[i].mIndices[j] );
		}
	}

	if ( !g_calculateLODs )
	{
		outLods.push_back ( srcIndices );
	}
	else
	{
		processLods ( srcIndices, srcVertices, outLods );
	}

	printf ( "\nCalculated LOD count: %u\n", (unsigned)outLods.size () );

	uint32_t numIndices = 0; 
	for ( size_t l = 0; l < outLods.size (); l++ )
	{
		for ( size_t i = 0; i < outLods[l].size(); i++ )
		{
			g_meshData.indexData_.push_back ( outLods[l][i] );
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

bool loadFile ( const char* filename )
{
	if ( verbose )
	{
		printf ( "Loading '%s'...\n", filename );
	}

	const unsigned int flags = 0 | aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_LimitBoneWeights |
		aiProcess_SplitLargeMeshes |
		aiProcess_ImproveCacheLocality |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_FindDegenerates |
		aiProcess_FindInvalidData |
		aiProcess_GenUVCoords;

	const aiScene* scene = aiImportFile ( filename, flags );

	if ( !scene || !scene->HasMeshes () )
	{
		printf ( "Unable to load '%s'\n", filename );
		return false;
	}

	g_meshData.meshes_.reserve ( scene->mNumMeshes );
	g_meshData.boxes_.reserve ( scene->mNumMeshes );

	for ( unsigned int i = 0; i != scene->mNumMeshes; i++ )
	{
		printf ( "\nConverting meshes %u/%u...", i + 1, scene->mNumMeshes );
		fflush ( stdout );
		g_meshData.meshes_.push_back ( convertAIMesh ( scene->mMeshes[i] ) );
	}

	recalculateBoundingBoxes ( g_meshData );

	return true;
}

int main ( int argc, char** argv )
{
	if ( argc < 3 )
	{
		printf ( "Usage: meshconvert <input> <output> [--export-texcoords | -t]	[--export-normals | -n]\n" );
		printf ( "Options: \n" );
		printf ( "\t--export-texcoords | -t: export texture coordinates\n" );
		printf ( "\t--export-normals | -n: export normals\n" );
		exit ( 255 );
	}

	for ( int i = 3; i < argc; i++ )
	{
		g_exportTextures |= !strcmp ( argv[i], "--export-texcoords" ) || !strcmp ( argv[i], "-t" );
		g_exportNormals |= !strcmp ( argv[i], "--export-normals" ) || !strcmp ( argv[i], "-n" );
		const bool exportAll = !strcmp ( argv[i], "-tn" ) || !strcmp ( argv[i], "-nt" );
		g_exportTextures |= exportAll;
		g_exportNormals |= exportAll;
	}

	for ( int i = 0; i < argc; i++ )
	{
		printf ( "Arg %i: %s\n", i, argv[i] );
	}

//	if ( g_exportTextures ) g_numElementsToStore += 2;
//	if ( g_exportNormals ) g_numElementsToStore += 3;

	printf ( "Attempting to load mesh file %s\n", argv[1] );

	if ( !loadFile ( argv[1] ) )
	{
		printf ( "Failed to load %s\n", argv[1] );
		exit ( 255 );
	}

	std::vector<DrawData> grid;
	g_vertexOffset = 0; 
	for ( auto i = 0; i < g_meshData.meshes_.size (); i++ )
	{
		grid.push_back ( DrawData{
			.meshIndex = static_cast<uint32_t>(i),
			.materialIndex = 0,
			.LOD = 0,
			.indexOffset = g_meshData.meshes_[i].indexOffset,
			.vertexOffset = g_vertexOffset,
			.transformIndex = 0
		} );
		g_vertexOffset += g_meshData.meshes_[i].vertexCount;
	}

	printf ( "Attempting to save processed mesh file %s\n", argv[2] );
	saveMeshData ( argv[2], g_meshData );

	std::string drawDataFilename = std::string ( argv[2] ).append ( ".drawData" );

	printf ( "Attempting to save DrawData for mesh %s to %s\n", argv[2], drawDataFilename.c_str () );
	FILE* f = fopen ( drawDataFilename.c_str (), "wb" );
	fwrite ( grid.data (), grid.size (), sizeof ( DrawData ), f );
	fclose ( f );

	return 0;
}