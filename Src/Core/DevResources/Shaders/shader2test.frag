#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push	
{
	mat4 transform;
	vec3 color;
} push;

void main() 
{
	// THIS SETS ALL COLOR TO BLUE
	outColor = vec4(0.0, 0.0, 1.0, 1.0);
}