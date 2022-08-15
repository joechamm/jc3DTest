#version 460 core 

layout (location = 0) out vec2 out_FragColor;

layout (binding = 0) uniform sampler2D VelocityTexture;
layout (binding = 3) uniform sampler2D TemperatureTexture;
layout (binding = 4) uniform sampler2D DensityTexture;

layout (location = 0) uniform float AmbientTemperature;
layout (location = 1) uniform float TimeStep;
layout (location = 2) uniform float Sigma;
layout (location = 3) uniform float Kappa;

void main()
{
	ivec2 TC = ivec2(gl_FragCoord.xy);
	float T = texelFetch(TemperatureTexture, TC, 0).r;
	vec2 V = texelFetch(VelocityTexture, TC, 0).xy;

	out_FragColor = V;

	if(T > AmbientTemperature)
	{
		float D = texelFetch(DensityTexture, TC, 0).x;
		out_FragColor += (TimeStep * (T - AmbientTemperature) * Sigma - D * Kappa) * vec2(0, 1);
	}
}