#version 450 core 

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec3 v_worldPos[];
layout (location = 1) in vec4 v_color[];

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

layout (location = 0) out vec3 g_worldPos;
layout (location = 1) out vec3 g_worldNormal;
layout (location = 2) out vec4 g_color;
layout (location = 3) out vec3 g_baryCoords;

const vec3 bc[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main()
{
	// opengl default front face winding order is ccw
	vec3 u = v_worldPos[1] - v_worldPos[0];
	vec3 v = v_worldPos[2] - v_worldPos[0];
	vec3 n = normalize(cross(u, v));
	for(int i = 0; i < 3; i++)
	{
		gl_Position = gl_in[i].gl_Position;
		g_worldPos = v_worldPos[i];
		g_worldNormal = n;
		g_color = v_color[i];
		g_baryCoords = bc[i];
		EmitVertex();
	}

	EndPrimitive();
}