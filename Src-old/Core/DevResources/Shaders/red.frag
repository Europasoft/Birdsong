#version 450
#extension GL_EXT_scalar_block_layout: require
// inputs from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWS;
layout(location = 2) in vec3 fragNormalWS;
layout(location = 3) in vec2 fragUV;

layout (location = 0) out vec4 outColor;


layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
} ubo1;

layout(set = 0, binding = 1) uniform texture2D textures[2];
layout(set = 0, binding = 2) uniform sampler _sampler;

layout(std430, set = 1, binding = 0) uniform UBO2 
{
	vec3 cameraPosition;
    vec3 lightPosition;
    float roughness;
} ubo2;

void main()
{
    outColor = vec4(1.0, 0.0, 0.0, 1.0); // just make red
}