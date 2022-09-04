#version 460 core 

layout (std140, binding = 7) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 model;
	uint velGridResX;
	uint velGridResY;
	float velLengthScale;
};

layout (std430, binding = 1) restrict readonly buffer Velocities
{
	vec2 in_Velocities[];
};

vec2 getVelocity(int idx) 
{
	return in_Velocities[idx];
}

vec2 getPosition(int idx)
{
	uint i = (idx - 1) % velGridResX;
	uint j = ((idx - i) / velGridResX) + 1;
	return vec2(i, j);
}

layout (location = 0) out vec2 position;
layout (location = 1) out vec2 velocity;

void main()
{
	int idx = gl_VertexID;
	vec2 pos = getPosition(idx);
	vec2 vel = getVelocity(idx);
	position = pos;
	velocity = vel;
	gl_Position = vec4(pos, 0.0, 1.0);
}