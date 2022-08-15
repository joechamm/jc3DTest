#version 460 core 

layout (location = 0) out vec4 out_FragColor;

layout (binding = 2) uniform sampler2D ObstaclesTexture;
layout (binding = 5) uniform sampler2D PressureTexture;
layout (binding = 6) uniform sampler2D DivergenceTexture;

layout (location = 0) uniform float Alpha;
layout (location = 1) uniform float InverseBeta;


void main()
{
	ivec2 T = ivec2(gl_FragCoord.xy);

	// Find neighboring pressure 
	vec4 pN = texelFetchOffset(PressureTexture, T, 0, ivec2(0, 1));
	vec4 pS = texelFetchOffset(PressureTexture, T, 0, ivec2(0, -1));
	vec4 pE = texelFetchOffset(PressureTexture, T, 0, ivec2(1, 0));
	vec4 pW = texelFetchOffset(PressureTexture, T, 0, ivec2(-1, 0));
	vec4 pC = texelFetch(PressureTexture, T, 0);

	// Find neighboring obstacles 
	vec3 oN = texelFetchOffset(ObstaclesTexture, T, 0, ivec2(0, 1)).rgb;
	vec3 oS = texelFetchOffset(ObstaclesTexture, T, 0, ivec2(0, -1)).rgb;
	vec3 oE = texelFetchOffset(ObstaclesTexture, T, 0, ivec2(1, 0)).rgb;
	vec3 oW = texelFetchOffset(ObstaclesTexture, T, 0, ivec2(-1, 0)).rgb;

	// use center pressure for solid cells 
	if(oN.x > 0) pN = pC;
	if(oS.x > 0) pS = pC;
	if(oE.x > 0) pE = pC;
	if(oW.x > 0) pW = pC;

	vec4 bc = texelFetch(DivergenceTexture, T, 0);
	out_FragColor = (pW + pE + pS + pN + Alpha * bc) * InverseBeta;
}