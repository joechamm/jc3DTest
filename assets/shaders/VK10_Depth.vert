//
#version 460

layout(location = 0) out vec3 uvw;
layout(location = 1) out vec3 v_worldNormal;
layout(location = 2) out vec4 v_worldPos;
layout(location = 3) out flat uint matIdx;

//

#extension GL_EXT_shader_explicit_arithmetic_types_int64: enable

struct ImDrawVert   { float x, y, z; float u, v; float nx, ny, nz; };
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

layout(binding = 0) uniform  UniformBuffer { mat4 proj; mat4 view; vec4 cameraPos; } ubo;
layout(binding = 1) readonly buffer SBO    { ImDrawVert data[]; } sbo;
layout(binding = 2) readonly buffer IBO    { uint   data[]; } ibo;
layout(binding = 3) readonly buffer DrawBO { DrawData data[]; } drawDataBuffer;
layout(binding = 5) readonly buffer XfrmBO { mat4 data[]; } transformBuffer;

void main()
{
	DrawData dd = drawDataBuffer.data[gl_BaseInstance];

	uint refIdx = dd.indexOffset + gl_VertexIndex;
	ImDrawVert v = sbo.data[ibo.data[refIdx] + dd.vertexOffset];

	mat4 model = transformBuffer.data[gl_BaseInstance];

	v_worldPos   = model * vec4(v.x, v.y, v.z, 1.0);
	v_worldNormal = transpose(inverse(mat3(model))) * vec3(v.nx, v.ny, v.nz);

	v_worldPos.y = -v_worldPos.y;

	/* Assign shader outputs */
	gl_Position = ubo.proj * ubo.view * v_worldPos;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	matIdx = dd.material;
	uvw = vec3(v.u, v.v, 1.0);
}