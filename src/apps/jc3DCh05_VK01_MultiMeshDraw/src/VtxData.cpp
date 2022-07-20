#include "VtxData.h"

#include <algorithm>
#include <assert.h>
#include <stdio.h>

MeshFileHeader loadMeshData ( const char* meshFilename, MeshData& out )
{
	MeshFileHeader header;
	/* Open mesh file as raw binary*/
	FILE* f = fopen ( meshFilename, "rb" );

	assert ( f );
	/* read the header file */
	if ( fread ( &header, 1, sizeof ( header ), f ) != sizeof ( header ) )
	{
		printf ( "Unable to read mesh file %s header info\n", meshFilename );
		exit ( 255 );
	}
	/* resize the mesh descriptors arrray and read in all the Mesh descriptions */
	out.meshes_.resize ( header.meshCount );
	if ( fread ( out.meshes_.data (), sizeof ( Mesh ), header.meshCount, f ) != header.meshCount )
	{
		printf ( "Could not read mesh descriptors from %s\n", meshFilename );
		exit ( 255 );
	}

	out.indexData_.resize ( static_cast<size_t>(header.indexDataSize / sizeof(uint32_t)) );
	out.vertexData_.resize ( static_cast<size_t>(header.vertexDataSize / sizeof(float)) );

	if ( (fread ( out.indexData_.data (), 1, header.indexDataSize, f ) != header.indexDataSize) ||
		(fread ( out.vertexData_.data (), 1, header.vertexDataSize, f ) != header.vertexDataSize) )
	{
		printf ( "Unable to read index/vertex data from %s\n", meshFilename );
		exit ( 255 );
	}

	fclose ( f );

	return header;
}