#version 460 core

layout (location = 0) in vec4 aPos;

layout(std140, binding = 0) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

// extents of grid in world coordinates
float gridSize = 100.0;

// size of one cell
float gridCellSize = 0.025;

// color of thin lines
vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.0);

// color of thick lines (every tenth line)
vec4 gridColorThick = vec4(0.0, 0.0, 0.0, 1.0);

// minimum number of pixels between cell lines before LOD switch should occur.
const float gridMinPixelsBetweenCells = 2.0;

const vec3 pos[4] = vec3[4](
	vec3(-1.0, 0.0, -1.0),
	vec3( 1.0, 0.0, -1.0),
	vec3( 1.0, 0.0,  1.0),
	vec3(-1.0, 0.0,  1.0)
);

const int indices[6] = int[6](
	0, 1, 2, 2, 3, 0
);

layout (location = 0) out vec2 uv;

void main()
{
	mat4 MVP = proj * view;

//	int idx = indices[gl_VertexID];
//	vec3 position = pos[idx] * gridSize;
	vec3 position = aPos.xyz * gridSize;
	int idx = indices[gl_VertexID];

	gl_Position = MVP * vec4(position, 1.0);
	uv = position.xz;
}