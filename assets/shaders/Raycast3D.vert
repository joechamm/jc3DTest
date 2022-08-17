#version 460 core 

layout (location = 0) in vec4 Position;
layout (location = 0) out vec4 vPosition;

layout (location = 10) uniform mat4 ModelViewProjection;

void main()
{
	gl_Position = ModelViewProjection * Position;
	vPosition = Position;
}