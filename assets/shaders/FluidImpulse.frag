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

void main()
{
	float d = distance(point, gl_FragCoord.xy);
	if(d < radius) 
	{
		float a = (radius - d) * 0.5;
		a = min(a, 1.0);
		out_FragColor = vec4(fillColor, a);
	} else {
		out_FragColor = vec4(0.0);
	}
}