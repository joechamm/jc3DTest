#version 450 core 

layout (location = 0) out vec4 out_FragColor;

layout (location = 0) in GS_FS_VERTEX 
{
	vec3 normal;
	vec3 worldPosition;
	float velocity;
} vertex_in;

const vec3 MinVelocityColor = vec3(1.0, 0.0, 0.0);
const vec3 MaxVelocityColor = vec3(0.0, 1.0, 0.0);

void main()
{
	float bias = vertex_in.velocity * 0.5 + 0.5;
	vec3 velColor = mix(MinVelocityColor, MaxVelocityColor, bias);
	out_FragColor = vec4(velColor, 1.0);
}

