#version 460 core 

layout (location = 0) out vec3 out_FragColor;
layout (location = 0) in float gLayer;

layout (binding = 0) uniform sampler3D VelocityTexture;
layout (binding = 2) uniform sampler3D Obstacles;
layout (binding = 3) uniform sampler3D Pressure;

layout (location = 0) uniform float GradientScale;

void main()
{
	ivec3 T = ivec3(gl_FragCoord.xy, gLayer);

	vec3 oC = texelFetch(Obstacles, T, 0).xyz;
	if(oC.x > 0)
	{
		out_FragColor = oC.yzx;
		return;
	}

	// Find neighboring pressure 
	float pN = texelFetchOffset(Pressure, T, 0, ivec3(0,  1, 0)).r;
	float pS = texelFetchOffset(Pressure, T, 0, ivec3(0, -1, 0)).r;
	float pE = texelFetchOffset(Pressure, T, 0, ivec3( 1, 0, 0)).r;
	float pW = texelFetchOffset(Pressure, T, 0, ivec3(-1, 0, 0)).r;
	float pU = texelFetchOffset(Pressure, T, 0, ivec3(0, 0,  1)).r;
	float pD = texelFetchOffset(Pressure, T, 0, ivec3(0, 0, -1)).r;
	float pC = texelFetch(Pressure, T, 0).r;

	// Find neighboring obstacles 
	vec3 oN = texelFetchOffset(Obstacles, T, 0, ivec3(0,  1, 0)).xyz;
	vec3 oS = texelFetchOffset(Obstacles, T, 0, ivec3(0, -1, 0)).xyz;
	vec3 oE = texelFetchOffset(Obstacles, T, 0, ivec3( 1, 0, 0)).xyz;
	vec3 oW = texelFetchOffset(Obstacles, T, 0, ivec3(-1, 0, 0)).xyz;
	vec3 oU = texelFetchOffset(Obstacles, T, 0, ivec3(0, 0,  1)).xyz;
	vec3 oD = texelFetchOffset(Obstacles, T, 0, ivec3(0, 0, -1)).xyz;

	// Use center pressure for solid cells 
	vec3 obstV = vec3(0);
	vec3 vMask = vec3(1);

	if(oN.x > 0)
	{
		pN = pC;
		obstV.y = oN.z;
		vMask.y = 0;
	}
	if(oS.x > 0) 
	{
		pS = pC; 
		obstV.y = oS.z;
		vMask.y = 0;
	}
	if(oE.x > 0)
	{
		pE = pC;
		obstV.x = oE.y;
		vMask.x = 0;
	}
	if(oW.x > 0)
	{
		pW = pC;
		obstV.x = oW.y;
		vMask.x = 0;
	}
	if(oU.x > 0)
	{
		pU = pC;
		obstV.z = oU.x;
		vMask.z = 0;
	}
	if(oD.x > 0)
	{
		pD = pC;
		obstV.z = oD.x;
		vMask.z = 0;
	}

	// Enforce the free-slip boundary condition 
	vec3 oldV = texelFetch(VelocityTexture, T, 0).xyz;
	vec3 grad = vec3(pE - pW, pN - pS, pU - pD) * GradientScale;
	vec3 newV = oldV - grad;
	out_FragColor = (vMask * newV) + obstV;
}
