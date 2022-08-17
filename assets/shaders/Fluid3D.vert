#version 460 core 

layout (location = 0) in vec4 Position;
layout (location = 0) out int vInstance;

void main()
{
	gl_Position = Position;
	vInstance = gl_InstanceID;
}