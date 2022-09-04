#version 450 core 

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

layout (std430, binding = 5) restrict readonly buffer Matrices 
{
	mat4 in_Model[];
};

layout (location = 0) in vec4 in_vPosition;
layout (location = 1) in vec4 in_vColor;

layout (location = 0) out vec3 v_worldPos;
layout (location = 1) out vec4 v_color;

void main()
{
	mat4 modelMat = in_Model[gl_InstanceID];
	vec4 pos = modelMat * in_vPosition;
	v_worldPos = pos.xyz;
	v_color = in_vColor;
	gl_Position = proj * view * pos;
}