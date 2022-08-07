//
#version 460 core 

struct Vertex
{
	float p[3];
	float n[3];
	float tc[2];
};

layout(std430, binding = 1) restrict readonly buffer Vertices
{
	Vertex in_Vertices[];
};

layout(std430, binding = 2) restrict readonly buffer Matrices
{
	mat4 in_ModelMatrices[];
};

layout (std140, binding = 0) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
};

vec3 getPosition(int i)
{
	return vec3(in_Vertices[i].p[0], in_Vertices[i].p[1], in_Vertices[i].p[2]);
}

void main()
{
	mat4 MVP = proj * view * in_ModelMatrices[gl_BaseInstance];

	gl_Position = MVP * vec4(getPosition(gl_VertexID), 1.0);
}