#version 450
#extension GL_EXT_scalar_block_layout: require
// input from vertex shader
layout(location = 0) in vec2 fragUV;

layout(push_constant) uniform Push
{
	vec2 position;
	vec2 size;
	float timeSinceHover;
	float timeSinceClick;
} push;

layout (location = 0) out vec4 outColor;

void main()
{
	if (push.timeSinceHover < 0.1) 
	{
		outColor = vec4(push.timeSinceHover,0.0,1.0,1.0);
	}
	else 
	{
		outColor = vec4(push.timeSinceHover,1.0,1.0,1.0);
	}
}