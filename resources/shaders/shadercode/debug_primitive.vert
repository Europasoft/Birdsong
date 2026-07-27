#version 450
#extension GL_EXT_scalar_block_layout: require
// vertex inputs
layout(location = 0) in vec4 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
// outputs to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPositionWS;
layout(location = 2) out vec3 fragNormalWS;
layout(location = 3) out vec2 fragUV;

layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
} ubo1;

layout(push_constant) uniform Push
{
	mat4 transform;
	vec4 color;
} push;


void main()
{
	gl_Position = ubo1.projectionViewMatrix * push.transform * position;
	fragPositionWS = vec4(push.transform * position).xyz;
	fragColor = push.color.xyz;
}