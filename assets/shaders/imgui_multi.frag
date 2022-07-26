#version 460
#extension GL_EXT_nonuniform_qualifier : require 
layout (location = 0) in vec2 uv;
layout (location = 1) in vec4 color;
layout (location = 0) out vec4 outColor;
layout (binding = 3) uniform sampler2D textures[];

layout (push_constant) uniform pushBlock { uint index; } pushConsts;

void main()
{
	const uint kDepthTextureMask = 0xFFFF;
	uint texType = (pushConsts.index >> 16) & kDepthTextureMask;
	uint tex = pushConsts.index & kDepthTextureMask;
	vec4 value = texture(textures[nonuniformEXT(tex)], uv);
	outColor = (texType == 0) ? (color * value) : vec4(value.rrr, 1.0);
}