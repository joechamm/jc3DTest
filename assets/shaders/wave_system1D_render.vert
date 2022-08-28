#version 450 core 

struct Wave1DVert 
{
	float height;
	float velocity;
};

layout (std430, binding = 1) restrict readonly buffer WaveVerticesIn
{
	Wave1DVert in_Vertices[];
};

Wave1DVert getVert(uint i)
{
	return in_Vertices[i];
}

layout (location = 0) out VS_GS_VERTEX 
{
	float dx;
	float velocity;
} vertex_out;

void main()
{
	const int vertCount = in_Vertices.length();	
	const float dx = 1.0 / float(vertCount - 1);

	Wave1DVert vert = getVert(gl_VertexID);

	float x = float(gl_VertexID) * dx;
	float y = vert.height;
	vertex_out.dx = dx;
	vertex_out.velocity = vert.velocity;
	gl_Position = vec4(x, y, 0.0, 1.0);
}