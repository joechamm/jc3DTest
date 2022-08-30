#version 450 core 

layout (location = 0) in vec4 aPosition;
layout (location = 1) in vec4 aColor;

struct Wave1DVert 
{
	float height;
	float velocity;
};

layout (std430, binding = 1) restrict readonly buffer WaveVerticesIn
{
	Wave1DVert in_Vertices[];
};

layout (std430, binding = 2) restrict readonly buffer WaveMatricesIn
{
	mat4 in_Model[];
};

layout (std140, binding = 7) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 model;
	vec4 cameraPos;
	uint vertCount;
	float boxPercentage;
};

layout (location = 0) out VS_GS_VERTEX 
{
	vec4	position;
	vec4	color;
	float	dx;
	float	height;
	float	velocity;
} vertex_out;

void main()
{
	mat4 modelToWorld = in_Model[gl_InstanceID];
	vertex_out.position = modelToWorld * aPosition;
	vertex_out.color = aColor;
	vertex_out.dx = 1.0 / float(vertCount - 1);
	vertex_out.height = in_Vertices[gl_VertexID].height;
	vertex_out.velocity = in_Vertices[gl_VertexID].velocity;
	gl_Position = proj * view * model * modelToWorld * aPosition;
}