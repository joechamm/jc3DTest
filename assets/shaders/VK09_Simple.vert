//
#version 460 

layout (location = 0) out vec3 v_worldNormal;
layout (location = 1) out vec4 v_worldPosition;

#extension GL_EXT_shader_explicit_arithmetic_types_int64: enable

struct ImDrawVert { float x, y, z; float u, v; float nx, ny, nz; };
struct DrawData {
	uint mesh;
	uint material;
	uint lod;
	uint indexOffset;
	uint vertexOffset;
	uint transformIndex;
};

struct MaterialData
{
	vec4 emissiveColor_;
	vec4 albedoColor_;
	vec4 roughness_;

	float transparencyFactor_;
	float alphaTest_;
	float metallicFactor_;

	uint  flags_;

	uint64_t ambientOcclusionMap_;
	uint64_t emissiveMap_;
	uint64_t albedoMap_;
	uint64_t metallicRoughnessMap_;
	uint64_t normalMap_;
	uint64_t opacityMap_;
};

layout (binding = 0) uniform UniformBuffer { mat4 proj; mat4 view; vec4 cameraPos; } ubo;
layout (binding = 1) readonly buffer SBO { ImDrawVert data[]; } sbo;
layout (binding = 2) readonly buffer IBO { uint data[]; } ibo;
layout (binding = 3) readonly buffer DrawBO { DrawData data[]; } drawDataBuffer;
layout (binding = 5) readonly buffer XfrmBO { mat4 data[]; } transformBuffer;


const vec3 pos[8] = vec3[8](
	vec3(-1.0,-1.0, 1.0),
	vec3( 1.0,-1.0, 1.0),
	vec3( 1.0, 1.0, 1.0),
	vec3(-1.0, 1.0, 1.0),

	vec3(-1.0,-1.0,-1.0),
	vec3( 1.0,-1.0,-1.0),
	vec3( 1.0, 1.0,-1.0),
	vec3(-1.0, 1.0,-1.0)
);

const uint indices[36] = uint[36](
	// front
	0, 1, 2, 2, 3, 0,
	// right
	1, 5, 6, 6, 2, 1,
	// back
	7, 6, 5, 5, 4, 7,
	// left
	4, 0, 3, 3, 7, 4,
	// bottom
	4, 5, 1, 1, 0, 4,
	// top
	3, 2, 6, 6, 7, 3
);

const vec3 normals[6] = vec3[6](
	vec3( 0, 0, 1),
	vec3( 1, 0, 0),
	vec3( 0, 0,-1),
	vec3(-1, 0,-1),
	vec3( 0,-1, 0),
	vec3( 0, 1, 0)
);

void main()
{
	uint vertexIndex = gl_VertexIndex % 36;
	
	uint vidx = indices[vertexIndex];
	uint faceIndex = vertexIndex / 6;

	vec3 position = pos[vidx];
	vec3 normal = normals[faceIndex];

	mat4 model = transformBuffer.data[gl_BaseInstance];

	v_worldPosition = model * vec4(position, 1.0);
	v_worldNormal = transpose(inverse(mat3(model))) * normal;

	// Flip Y axis (Vulkan compatibilitiy
	v_worldPosition.y = - v_worldPosition.y;

	gl_Position = ubo.proj * ubo.view * v_worldPosition;
}