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
layout (binding = 7) uniform sampler2D ObstaclesTexture;
layout (binding = 8) uniform sampler2D PressureTexture;

void main()
{
	ivec2 T = ivec2(gl_FragCoord.xy);

	vec3 oC = texelFetch(ObstaclesTexture, T, 0).xyz;
	if(oC.x > 0) 
	{
		out_FragColor = oC.yz;
		return;
	}

	// Find neighboring pressure 
	float pN = texelFetchOffset(PressureTexture, T, 0, ivec2(0, 1)).r;
	float pS = texelFetchOffset(PressureTexture, T, 0, ivec2(0, -1)).r;
	float pE = texelFetchOffset(PressureTexture, T, 0, ivec2(1, 0)).r;
	float pW = texelFetchOffset(PressureTexture, T, 0, ivec2(-1, 0)).r;
	float pC = texelFetch(PressureTexture, T, 0).r;

	// Find neighboring obstacles 
	vec3 oN = texelFetchOffset(ObstaclesTexture, T, 0, ivec2(0, 1)).xyz;
	vec3 oS = texelFetchOffset(ObstaclesTexture, T, 0, ivec2(0, -1)).xyz;
	vec3 oE = texelFetchOffset(ObstaclesTexture, T, 0, ivec2(1, 0)).xyz;
	vec3 oW = texelFetchOffset(ObstaclesTexture, T, 0, ivec2(-1, 0)).xyz;

	// Use center pressure for solid cells
	vec2 obstV = vec2(0);
	vec2 vMask = vec2(1);

	if(oN.x > 0) { pN = pC; obstV.y = oN.z; vMask.y = 0; }
	if(oS.x > 0) { pS = pC; obstV.y = oS.z; vMask.y = 0; }
	if(oE.x > 0) { pE = pC; obstV.x = oE.y; vMask.x = 0; }
	if(oW.x > 0) { pW = pC; obstV.x = oW.y; vMask.x = 0; }

	// Enforce the free slip boundary condition 
	vec2 oldV = texelFetch(VelocityTexture, T, 0).xy;
	vec2 grad = vec2(pE - pW, pN - pS) * gradientScale;
	vec2 newV = oldV - grad;
	out_FragColor = (vMask * newV) + obstV;
}