#ifndef __VTX_DATA_H__
#define __VTX_DATA_H__

#include <stdint.h>

#include <glm/glm.hpp>

#include "Utils.h"
#include "UtilsMath.h"

/**
 * @brief we need two constants to define the limits on how many LODs and vertex streams we can have in a single mesh
*/

constexpr const uint32_t kMaxLODs = 8;
constexpr const uint32_t kMaxStreams = 8;

/**
 * @brief Define an individual mesh description. 
 * We deliberately avoid using pointers that hide memory allocations and prohibit the simple saving and loading of data. 
 * We store offsets to individual data streams and LOD index buffers. They are equivalent to pointers but are more flexible and, most importantly, GPU-friendlier. 
 * All the offsets in the Mesh structure are given relative to the beginning of the data block.
 * The LOD count, where the original mesh counts as one of the LODs, must be strictly less than kMaxLODs. This is because we do not store LOD index buffer sizes but calculate them from offsets. 
 * To calculate these sizes, we store one additional empty LOD level at the end. The number of vertex data streams is stored directly with no modifications.
*/

struct Mesh final
{
	uint32_t lodCount;
	uint32_t streamCount;

	/* The materialID field contains an abstract identifier that allows us to reference any material data that is stored elsewhere */
	uint32_t materialID;

	/**
	 * @brief The meshSize field must be equal to the sum of all LOD index array sizes and the sum of all individuals stream sizes. 
	*
	* The vertexCount field contains the total number of vertices in this mesh. This number can be greater than the number of vertices on any individual LOD. 
	*/
	uint32_t meshSize;
	uint32_t vertexCount;

	/* Each mesh can potentially be displayed at different LODs. The file contains all the indices for all the LODs, and offsets to the beginning of each LOD are stored in the lodOffset array. This array contains one extra item at the end, which serves as a marker to calculate the size of the last LOD */
	uint32_t lodOffset[kMaxLODs];

	/* Instead of storing the sizes of each LOD, we define a little helper function to calculate their sizes */
	inline uint64_t lodSize ( uint32_t lod )
	{
		return lodOffset[lod + 1] - lodOffset[lod];
	}
	
	/* streamOffset field stores offsets to all of the individual vertex data streams */
	/* IMPORTANT NOTE Besides the element size, we might want to store the element type, such as byte, short integer, or float. This information is important for performance reasons in real-world applications. To simplify the code in this book, we will not do it here */
	uint64_t streamOffset[kMaxStreams];
	uint32_t streamElementSize[kMaxStreams];
};

struct MeshFileHeader
{
	/* To ensure data integrity and to check the validity of the header, a magic hexadecimal value of 0x12345678 is stored in the first 4 bytes of the header */
	uint32_t magicValue;

	/* The number of different meshes in this file is stored in meshCount */
	uint32_t meshCount;

	/* For convenience, store an offset to the the beginning of the mesh data */
	uint32_t dataBlockStartOffset;

	/* store the sizes of index and vertex data in bytes to check the integrity of a mesh file */
	uint32_t indexDataSize;
	uint32_t vertexDataSize;
};

struct InstanceData
{
	float transform[16];
	uint32_t meshIndex;
	uint32_t materialIndex;
	uint32_t LOD;
	uint32_t indexOffset;
};

struct MeshData
{
	std::vector<Mesh> meshes_;
	std::vector<uint8_t> indexData_;
	std::vector<uint8_t> vertexData_;
};

MeshFileHeader loadMeshData ( const char* meshFilename, MeshData& out );

#endif // __VTX_DATA_H__