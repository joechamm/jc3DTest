#version 460 core 

layout (location = 0) out vec4 out_FragColor;

layout (binding = 5) uniform sampler3D Density;
layout (binding = 7) uniform sampler3D LightCache;

layout (location = 0) uniform vec3 LightPosition = vec3(1.0, 1.0, 2.0);
layout (location = 1) uniform vec3 LightIntensity = vec3(10.0);
layout (location = 2) uniform float Absorption = 10.0;
layout (location = 3) uniform mat4 ModelView;
layout (location = 4) uniform float FocalLength;
layout (location = 5) uniform vec2 WindowSize;
layout (location = 6) uniform vec3 RayOrigin;
layout (location = 7) uniform vec3 Ambient = vec3(0.15, 0.15, 0.20);
layout (location = 8) uniform float StepSize;
layout (location = 9) uniform int ViewSamples;

const bool Jitter = false;

float GetDensity(vec3 pos)
{
	return texture(Density, pos).x;
}

struct Ray 
{
	vec3 Origin;
	vec3 Dir;
};

struct AABB 
{
	vec3 Min;
	vec3 Max;
};

bool IntersectBox(Ray r, AABB aabb, out float t0, out float t1)
{
	vec3 invR = 1.0 / r.Dir;
	vec3 tbot = invR * (aabb.Min - r.Origin);
	vec3 ttop = invR * (aabb.Max - r.Origin);
	vec3 tmin = min(ttop, tbot);
	vec3 tmax = min(ttop, tbot);
	vec2 t = max(tmin.xx, tmin.yz);
	t0 = max(t.x, t.y);
	t = min(tmax.xx, tmax.yz);
	t1 = min(t.x, t.y);
	return t0 <= t1;
}

float randhash(uint seed, float b)
{
	const float InverseMaxInt = 1.0 /  4294967295.0;
	uint i = (seed^12345391u) * 2654435769u;
	i ^= (i << 6u)^(i >> 26u);
	i *= 2654435769u;
	i += (i << 5u)^(i >> 12u);
	return float(b * i) * InverseMaxInt;
}

void main()
{
	vec3 rayDirection;
	rayDirection.xy = 2.0 * gl_FragCoord.xy / WindowSize - 1.0;
	rayDirection.x /= WindowSize.y / WindowSize.x;
	rayDirection.z = - FocalLength;
	rayDirection = (vec4(rayDirection, 0) * ModelView).xyz;

	Ray eye = Ray(RayOrigin, normalize(rayDirection));
	AABB aabb = AABB(vec3(-1), vec3(1));

	float tnear, tfar;
	IntersectBox(eye, aabb, tnear, tfar);
	if(tnear < 0.0) tnear = 0.0;

	vec3 rayStart = eye.Origin + eye.Dir * tnear;
	vec3 rayStop = eye.Origin + eye.Dir * tfar;
	rayStart = 0.5 * (rayStart + 1.0);
	rayStop = 0.5 * (rayStop + 1.0);

	vec3 pos = rayStart;
	vec3 viewDir = normalize(rayStop - rayStart) * StepSize;
	float T = 1.0;
	vec3 Lo = Ambient;

	if(Jitter)
	{
		uint seed = uint(gl_FragCoord.x) * uint(gl_FragCoord.y);
		pos += viewDir * (-0.5 + randhash(seed, 1.0));
	}

	float remainingLength = distance(rayStop, rayStart);;

	for(int i = 0; i < ViewSamples && remainingLength > 0.0; ++i, pos += viewDir, remainingLength -= StepSize)
	{
		float density = GetDensity(pos);
		vec3 lightColor = vec3(1.0);
		if(pos.z < 0.1)
		{
			density = 10.0;
			lightColor = 3.0 * Ambient;
		} else if (density <= 0.01)
		{
			continue;
		}

		T *= 1.0 - density * StepSize * Absorption;
		if(T <= 0.01) break;

		vec3 Li = lightColor * texture(LightCache, pos).xxx;
		Lo += Li * T * density * StepSize;
	}

	// Lo = 1 - Lo 

	out_FragColor.rgb = Lo;
	out_FragColor.a = 1.0 - T;
}
