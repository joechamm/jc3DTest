#version 460 core 

layout (location = 0) out vec4 out_FragColor;

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

layout (binding = 12) uniform sampler2D SamplerTexture;

void main()
{
	float L = texture(SamplerTexture, gl_FragCoord.xy * scale).r;
	out_FragColor = vec4(fillColor, L);
}