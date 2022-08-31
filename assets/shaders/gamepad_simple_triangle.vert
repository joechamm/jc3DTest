#version 450 core 

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec4 inColor;

layout (std140, binding = 7) uniform PerFrameData
{
	mat4 proj;
	mat4 view;
	mat4 model;
};

layout (std140, binding = 8) uniform TriangleOffsets 
{
	vec2 offsets[16];
};

const vec2 positionOffsets[4] = vec2[](
	vec2(-0.5, -0.5),
	vec2( 0.5f,-0.5),
	vec2(-0.5,  0.5),
	vec2( 0.5,  0.5)
	);

layout (location = 0) out vec4 vColor;

void main()
{
//	uint row = gl_InstanceID / 4;
//	uint col = gl_InstanceID % 4;
//	float roff = float(row) / 4.0;
//	float goff = float(col) / 4.0;
//	float boff = float(gl_InstanceID) / 16.0f;
//	float rval = mod(inColor.r + roff, 1.0);
//	float gval = mod(inColor.g + goff, 1.0);
//	float bval = mod(inColor.b + boff, 1.0);
	vec4 pos = inPosition;
	vec2 off = positionOffsets[gl_InstanceID];
	pos.xy += off;
	mat4 mvp = proj * view * model;
	vColor = inColor;
	gl_Position = mvp * pos;
}