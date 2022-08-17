#version 460 core 

layout (location = 0) out float out_FragColor;
layout (location = 0) in float gLayer;

layout (binding = 5) uniform sampler3D Density;

layout (location = 0) uniform vec3 LightPosition = vec3(1.0, 1.0, 2.0);
layout (location = 1) uniform float LightIntensity = 10.0;
layout (location = 2) uniform float Absorbtion = 10.0;
layout (location = 3) uniform float LightStep;
layout (location = 4) uniform int LightSamples;
layout (location = 5) uniform vec3 InverseSize;

float GetDensity(vec3 pos)
{
	return texture(Density, pos).x;
}

void main()
{
	vec3 pos = InverseSize * vec3(gl_FragCoord.xy, gLayer);
	vec3 lightDir = normalize(LightPosition - pos) * LightStep;
	float Tl = 1.0;
	vec3 lpos = pos + lightDir;

	for(int s = 0; s < LightSamples; s++) 
	{	
		float ld = GetDensity(lpos);
		Tl *= 1.0 - Absorbtion * LightStep * ld;
		if(Tl <= 0.01) break;

		// Would be faster if this conditional is replaced with a tigher loop
		if(lpos.x < 0 || lpos.y < 0 || lpos.z < 0 || lpos.x > 1 || lpos.y > 1 || lpos.z > 1) break;

		lpos += lightDir;
	} 

	float Li = LightIntensity * Tl;
	out_FragColor = Li;
}