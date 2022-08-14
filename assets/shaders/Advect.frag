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

layout (binding = 5) uniform sampler2D VelocityTexture;
layout (binding = 6) uniform sampler2D SourceTexture;
layout (binding = 7) uniform sampler2D ObstaclesTexture;

void main()
{
	vec2 fragCoord = gl_FragCoord.xy;
	
	float solid = texture(ObstaclesTexture, inverseSize * fragCoord).x;
	if(solid > 0) 
	{
		out_FragColor = vec4(0.0);
		return;
	}

	vec2 u = texture(VelocityTexture, inverseSize * fragCoord).xy;
	vec2 coord = inverseSize * (fragCoord - timeStep * u);
	out_FragColor = dissipation * texture(SourceTexture, coord);
}