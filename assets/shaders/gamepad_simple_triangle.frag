#version 450 core 

layout (location = 0) out vec4 out_FragColor;

layout (location = 0) in vec4 vColor;

void main()
{
	out_FragColor = vColor;
}