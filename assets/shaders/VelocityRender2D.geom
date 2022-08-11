#version 460 core 

layout (points) in;
layout (line_strip, max_vertices = 2) out;

layout (std140, binding = 0) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 model;
	uint velGridResX;
	uint velGridResY;
	float velLengthScale;
};

layout (location = 0) in vec2 in_positions[];
layout (location = 1) in vec2 in_velocities[];

void main()
{
	vec2 pos = in_positions[0];
	vec2 vel = in_velocities[0];
	
	vec2 velTipPos = pos + vel * velLengthScale;

	gl_Position = proj * view * model * vec4(pos, 0.0, 1.0);
	EmitVertex();

	gl_Position = proj * view * model * vec4(velTipPos, 0.0, 1.0);
	EmitVertex();
	EndPrimitive();
}