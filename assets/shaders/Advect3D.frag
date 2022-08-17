#version 460 core 

layout (location = 0) out vec4 out_FragColor;

layout (location = 0) in float gLayer;

layout (binding = 0) uniform sampler3D VelocityTexture;
layout (binding = 1) uniform sampler3D SourceTexture;
layout (binding = 2) uniform sampler3D Obstacles;

layout (location = 0) uniform float TimeStep;
layout (location = 1) uniform float Dissipation;
layout (location = 5) uniform vec3 InverseSize;

void main()
{
	vec3 fragCoord = vec3(gl_FragCoord.xy, gLayer);
	float solid = texture(Obstacles, InverseSize * fragCoord).x;
	if(solid > 0) 
	{
		out_FragColor = vec4(0.0);
		return;
	}

	vec3 u = texture(VelocityTexture, InverseSize * fragCoord).xyz;

	vec3 coord = InverseSize * (fragCoord - TimeStep * u);
	out_FragColor = Dissipation * texture(SourceTexture, coord);
}