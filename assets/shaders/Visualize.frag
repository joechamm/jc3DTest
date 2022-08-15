#version 460 core 

layout (location = 0) out vec4 out_FragColor;

layout (binding = 7) uniform sampler2D SamplerTexture;

layout (location = 0) uniform vec2 Scale;
layout (location = 1) uniform vec3 FillColor;

void main()
{
	float L = texture(SamplerTexture, gl_FragCoord.xy * Scale).r;
	out_FragColor = vec4(FillColor, L);
}