#version 460 core 

layout (location = 0) out vec2 out_FragColor;

layout (std140, binding = 0) uniform PerFrameData 
{
	vec3 fillColor;
	vec2 inverseSize;
	vec2 scale;
	vec2 point;
	float timeStep;
	float dissipation;
	float alpha;
	float inverseBeta;
	float gradientScale;
	float halfInverseCellSize;
	float radius;
	float ambientTemperature;
	float sigma;
	float kappa;
};

layout (binding = 5) uniform sampler2D VelocityTexture;
layout (binding = 10) uniform sampler2D TemperatureTexture;
layout (binding = 11) uniform sampler2D DensityTexture;

void main()
{
	ivec2 TC = ivec2(gl_FragCoord.xy);
	float T = texelFetch(TemperatureTexture, TC, 0).r;
	vec2 V = texelFetch(VelocityTexture, TC, 0).xy;

	out_FragColor = V;

	if(T > ambientTemperature)
	{
		float D = texelFetch(DensityTexture, TC, 0).x;
		out_FragColor += (timeStep * (T - ambientTemperature) * sigma - D * kappa) * vec2(0, 1);
	}
}