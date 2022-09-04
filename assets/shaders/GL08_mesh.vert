//
#version 460 core 

#extension GL_ARB_gpu_shader_int64 : enable 

//

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

layout (std140, binding = 7) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

layout (std430, binding = 5) restrict readonly buffer Matrices 
{
	mat4 in_Model[];
};

layout (location = 0) in vec3 in_Vertex;
layout (location = 1) in vec2 in_TexCoord;
layout (location = 2) in vec3 in_Normal;

layout (location = 0) out vec2 v_tc;
layout (location = 1) out vec3 v_worldNormal;
layout (location = 2) out vec3 v_worldPos;
layout (location = 3) out flat uint matIdx;

void main()
{
	mat4 model = in_Model[gl_InstanceID];
	mat4 MVP = proj * view * model;

	gl_Position = MVP * vec4(in_Vertex, 1.0);

	v_worldPos = (view * vec4(in_Vertex, 1.0)).xyz;
	v_worldNormal = transpose(inverse(mat3(model))) * in_Normal;
	v_tc = in_TexCoord;
	matIdx = gl_BaseInstance;
}