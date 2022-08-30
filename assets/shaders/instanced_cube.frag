#version 450 core 

layout (location = 0) out vec4 out_FragColor;

layout (std140, binding = 7) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
	vec4 lightPos;
	vec4 ambientLight;
	vec4 diffuseLight;
	vec4 specularLight;
};

layout (std430, binding = 0) restrict readonly buffer Matrices
{
	mat4 in_Model[];
};

layout (location = 0) in vec3 g_worldPos;
layout (location = 1) in vec3 g_worldNormal;
layout (location = 2) in vec4 g_color;
layout (location = 3) in vec3 g_baryCoords;

const float specular_power = 128.0f;

vec3 ambientContribution()
{
	return ambientLight.rgb * g_color.rgb;
}

vec3 diffuseContribution(vec3 surfNormal, vec3 objToLight)
{
	float diffuseCoefficient = max(dot(surfNormal, objToLight), 0.0);
	return diffuseLight.rgb * g_color.rgb * diffuseCoefficient;
}

vec3 specularContribution(vec3 surfNormal, vec3 objToLight, vec3 objToCam)
{
	vec3 reflectedDir = reflect(- objToLight, surfNormal);
	float rDotV = max(dot(reflectedDir, objToCam), 0.0);
	float specularCoefficient = pow(rDotV, specular_power);
	return specularLight.rgb * g_color.rgb * specularCoefficient;
}

float edgeFactor(float thickness)
{
	vec3 a3 = smoothstep(vec3(0.0), fwidth(g_baryCoords) * thickness, g_baryCoords);
	return min(min(a3.x, a3.y), a3.z);
}

void main()
{
	vec3 surfNormal = normalize(g_worldNormal);
	vec3 objToLight = normalize(lightPos.xyz - g_worldPos);
	vec3 objToCam = normalize(cameraPos.xyz - g_worldPos);
	vec3 ambientTerm = ambientContribution();
	vec3 diffuseTerm = diffuseContribution(surfNormal, objToLight);
	vec3 specularTerm = specularContribution(surfNormal, objToLight, objToCam);
	vec4 lightingColor = vec4(ambientTerm + diffuseTerm + specularTerm, 1.0);
	out_FragColor = mix(lightingColor * vec4(0.8), lightingColor, edgeFactor(1.0));
}