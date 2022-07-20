#version 460 

layout (location = 0) out vec3 uvw;

struct ImDrawVert { 
	float x, y, z; 
};

struct InstanceData {
	mat4 xfrm;
	uint mesh;
	uint matID;
	uint lod;
};

struct MaterialData { 
	uint tex2D; 
};

layout (binding = 0) uniform UniformBuffer { mat4 inMtx; } ubo;
layout (binding = 1) readonly buffer SBO { ImDrawVert data[]; } sbo;
layout (binding = 2) readonly buffer IBO { uint data[]; } ibo;
layout (binding = 3) readonly buffer InstBO { InstanceData data[]; } instanceBuffer;

vec3 colors[4] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, 0.0)
);

void main()
{
	uint idx = ibo.data[gl_VertexIndex];
	ImDrawVert v = sbo.data[idx];
	uvw = (gl_BaseInstance >= 4) ? vec3(1, 1, 0) : colors[gl_BaseInstance];
	mat4 xfrm = transpose(instanceBuffer.data[gl_BaseInstance].xfrm);
	gl_Position = ubo.inMtx * xfrm * vec4(v.x, v.y, v.z, 1.0);
}