#version 450 core 

layout (location = 0) in vec2 aHeightVelocity;

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
	float dx;
	float velocity;
} vertex_out;

void main()
{
	float dx = 1.0 / float(vertCount - 1);
	float x = float(gl_VertexID) * dx;
	float y = aHeightVelocity.x;
	vertex_out.dx = dx;
	vertex_out.velocity = aHeightVelocity.y;
	gl_Position = vec4(x, y, 0.0, 1.0);
}