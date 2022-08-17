#version 460 core 

layout (location = 0) out vec3 out_FragColor;
layout (location = 0) in float gLayer;

layout (binding = 0) uniform sampler3D Velocity;
layout (binding = 4) uniform sampler3D Temperature;
layout (binding = 5) uniform sampler3D Density;

layout (location = 0) uniform float TimeStep;
layout (location = 1) uniform float AmbientTemperature;
layout (location = 2) uniform float Sigma;
layout (location = 3) uniform float Kappa;

void main()
{
	ivec3 TC = ivec3(gl_FragCoord.xy, gLayer);
	float T = texelFetch(Temperature, TC, 0).r;
	vec3 V = texelFetch(Velocity, TC, 0).xyz;

	out_FragColor = V;

	if(T > AmbientTemperature) 
	{
		float D = texelFetch(Density, TC, 0).x;
		out_FragColor += (TimeStep * (T - AmbientTemperature) * Sigma - D * Kappa) * vec3(0, -1, 0);
	}
}