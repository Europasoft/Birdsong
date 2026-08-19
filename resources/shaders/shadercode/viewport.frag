#version 450
#extension GL_EXT_scalar_block_layout: require
layout(location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

// SCENE GLOBAL DESCRIPTOR SET
layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
    vec4 resolution; // viewport and swapchain resolutions in pixels
} ubo1;

// VIEWPORT DESCRIPTOR SET - ATTACHMENT FROM PREVIOUS PASS
layout(set = 1, binding = 0) uniform texture2D attachment;
// VIEWPORT DESCRIPTOR SET - SAMPLER
layout(set = 2, binding = 0) uniform sampler _attachmentSampler;

// VIEWPORT PUSH CONSTANTS
layout(push_constant) uniform PushConstants 
{
    vec4 positionAndSize;
} viewportPushConstants;

void main()
{
	outColor = texture(sampler2D(attachment, _attachmentSampler), inUV);
}