#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include "VtxData.h"

#include <meshoptimizer.h>

#ifndef NDEBUG
bool verbose = true;
#else
bool verbose = false;
#endif 

std::vector<Mesh> meshes;
std::vector<uint32_t> indexData;
std::vector<float> vertexData;

uint32_t indexOffset = 0;
uint32_t vertexOffset = 0;
bool exportTextures = false;
bool exportNormals = false;

uint32_t numElementsToStore = 3;

Mesh convertAIMesh ( const aiMesh* m )
{
	/* Check if there is a set of texture coordinates present */
	const bool hasTexCoords = m->HasTextureCoords ( 0 );

	/* We are assuming here that there is a single LOD and all the vertex data is stored as a continuous data stream. We will ignore material information for now. */
	const uint32_t numIndices = m->mNumFaces * 3;
	const uint32_t numElements = numElementsToStore;

	/* The size of the stream element in bytes is directly calculated from the number of elements per vertex. Since we are storing each component as a floating point value, no branching logic is required now. */
	const uint32_t streamElementSize = static_cast<uint32_t>(numElements * sizeof ( float ));

	/* Total data size is the size of the vertex stream + size of the index data */
	const uint32_t meshSize = static_cast<uint32_t>(m->mNumVertices * streamElementSize + numIndices * sizeof ( uint32_t ));
	
	/* The mesh descriptor for the aiMesh input object has its lodCount and streamCount fields set to 1. 
	*	Since there is only one LOD, the lodOffset array contains two items. 
	*	The first stores the indexOffset counter multiplied by a single index size to determine the byte offset in the indexData array. 
	*	The second element contains the last item of the indexData. 
	*/
	const Mesh result = {
		.lodCount = 1,
		.streamCount = 1,
		.materialID = 0,
		.meshSize = meshSize,
		.vertexCount = m->mNumVertices,
		.lodOffset = {
			indexOffset * sizeof ( uint32_t ),
			(indexOffset + numIndices) * sizeof ( uint32_t )
		},
		.streamOffset = { vertexOffset * streamElementSize },
		.streamElementSize = { streamElementSize }
	};

	/* For each of the vertices, we extract their data from the aiMesh object and always store vertex coordinates in the vertexData output stream */
	for ( size_t i = 0; i != m->mNumVertices; i++ )
	{
		const aiVector3D& v = m->mVertices[i];
		const aiVector3D& n = m->mNormals[i];
		const aiVector3D& t = hasTexCoords ? m->mTextureCoords[0][i] : aiVector3D ();
		
		vertexData.push_back ( v.x );
		vertexData.push_back ( v.y );
		vertexData.push_back ( v.z );

		if ( exportTextures )
		{
			vertexData.push_back ( t.x );
			vertexData.push_back ( t.y );
		}

		if ( exportNormals )
		{
			vertexData.push_back ( n.x );
			vertexData.push_back ( n.y );
			vertexData.push_back ( n.z );
		}
	}

	/* The vertexOffset variable contains the starting vertex index for the current mesh. We add the vertexOffset value to each of the indices imported */
	for ( size_t i = 0; i != m->mNumFaces; i++ )
	{
		const aiFace& F = m->mFaces[i];
		indexData.push_back ( F.mIndices[0] + vertexOffset );
		indexData.push_back ( F.mIndices[1] + vertexOffset );
		indexData.push_back ( F.mIndices[2] + vertexOffset );
	}

	indexOffset += numIndices;
	vertexOffset += m->mNumVertices;

	return result;
}

bool loadFile ( const char* filename )
{
	if ( verbose )
	{
		printf ( "Loading '%s'...\n", filename );
	}

	const unsigned int flags = 0 | aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate | aiProcess_GenSmoothNormals |
		aiProcess_PreTransformVertices | aiProcess_RemoveRedundantMaterials |
		aiProcess_FindDegenerates | aiProcess_FindInvalidData |
		aiProcess_FindInstances | aiProcess_OptimizeMeshes;

	const aiScene* scene = aiImportFile ( filename, flags );

	if ( !scene || !scene->HasMeshes () )
	{
		printf ( "Unable to load '%s'\n", filename );
		return false;
	}

	meshes.reserve ( scene->mNumMeshes );
	for ( size_t i = 0; i != scene->mNumMeshes; i++ )
	{
		meshes.push_back ( convertAIMesh ( scene->mMeshes[i] ) );
	}

	return true;
}

inline void saveMeshesToFile ( FILE* f )
{
	const MeshFileHeader header = {
		.magicValue = 0x12345678,
		.meshCount = static_cast<uint32_t>(meshes.size ()),
		.dataBlockStartOffset = static_cast<uint32_t>((sizeof ( MeshFileHeader ) + meshes.size () * sizeof ( Mesh ))),
		.indexDataSize = static_cast<uint32_t>(indexData.size () * sizeof ( uint32_t )),
		.vertexDataSize = static_cast<uint32_t>(vertexData.size () * sizeof ( float ))
	};

	fwrite ( &header, 1, sizeof ( header ), f );
	fwrite ( meshes.data (), header.meshCount, sizeof ( Mesh ), f );
	fwrite ( indexData.data (), 1, header.indexDataSize, f );
	fwrite ( vertexData.data (), 1, header.vertexDataSize, f );
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
		exportTextures |= !strcmp ( argv[i], "--export-texcoords" ) || !strcmp ( argv[i], "-t" );
		exportNormals |= !strcmp ( argv[i], "--export-normals" ) || !strcmp ( argv[i], "-n" );
		const bool exportAll = !strcmp ( argv[i], "-tn" ) || !strcmp ( argv[i], "-nt" );
		exportTextures |= exportAll;
		exportNormals |= exportAll;
	}

	if ( exportTextures ) numElementsToStore += 2;
	if ( exportNormals ) numElementsToStore += 3;

	if ( !loadFile ( argv[1] ) ) exit ( 255 );

	FILE* f = fopen ( argv[2], "wb" );
	saveMeshesToFile ( f );
	fclose ( f );
	return 0;
}