#version 460 core 

layout (location = 0) out float out_FragColor;

layout (location = 0) in float gLayer;

layout (binding = 5) uniform sampler3D Density;

layout (location = 8) uniform float StepSize;
layout (location = 1) uniform float DensityScale;
layout (location = 5) uniform vec3 InverseSize;

float GetDensity(vec3 pos) 
{
	return texture(Density, pos).x * DensityScale;
}

// This implements a super stupid filter in 3-space that can take 7 samples 
// A three-pass separable Gaussian would be way better 
void main()
{
	vec3 pos = InverseSize * vec3(gl_FragCoord.xy, gLayer);
	float e = StepSize;
	float z = e;
	float density = GetDensity(pos);
	density += GetDensity(pos + vec3(e, e, 0));
	density += GetDensity(pos + vec3(-e, e, 0));
	density += GetDensity(pos + vec3(e, -e, 0));
	density += GetDensity(pos + vec3(-e, -e, 0));
	density += GetDensity(pos + vec3(0, 0, -z));
	density += GetDensity(pos + vec3(0, 0, z));
	density /= 7;
	out_FragColor = density;
}