#version 460 core 

layout (location = 0) out vec4 out_FragColor;
layout (location = 0) in float gLayer;

layout (location = 0) uniform float Radius;
layout (location = 1) uniform vec3 FillColor;
layout (location = 2) uniform vec3 Point;

void main()
{
	float d = distance(Point, vec3(gl_FragCoord.xy, gLayer));
	if(d < Radius)
	{
		float a = (Radius - d) * 0.5;
		a = min(a, 1.0);
		out_FragColor = vec4(FillColor, a);
	} else {
		out_FragColor = vec4(0);
	}
}