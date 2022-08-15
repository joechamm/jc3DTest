#version 460 core 

layout (location = 0) out vec4 out_FragColor;

layout (binding = 0) uniform sampler2D VelocityTexture;
layout (binding = 1) uniform sampler2D SourceTexture;
layout (binding = 2) uniform sampler2D ObstaclesTexture;

layout (location = 0) uniform vec2 InverseSize;
layout (location = 1) uniform float TimeStep;
layout (location = 2) uniform float Dissipation;

void main()
{
	vec2 fragCoord = gl_FragCoord.xy;
	
	float solid = texture(ObstaclesTexture, InverseSize * fragCoord).x;
	if(solid > 0) 
	{
		out_FragColor = vec4(0.0);
		return;
	}

	vec2 u = texture(VelocityTexture, InverseSize * fragCoord).xy;
	vec2 coord = InverseSize * (fragCoord - TimeStep * u);
	out_FragColor = Dissipation * texture(SourceTexture, coord);
}