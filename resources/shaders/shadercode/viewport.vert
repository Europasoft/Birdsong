#version 450
#extension GL_EXT_scalar_block_layout: require
layout(location = 0) out vec2 outUV;
// no vertex inputs, vertices are generated here

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
} push;

void main() 
{
    // the viewport is a simple rectangle drawn at the position and size specified in the push constants
    // the fragment shader will sample the attachment image for final visualization
    vec2 viewportExtent = ubo1.resolution.xy;
    vec2 swapchainExtent = ubo1.resolution.zw;
    vec2 offsetPixels = push.positionAndSize.xy;
    vec2 sizePixels = push.positionAndSize.zw;

	vec2 uvs[6] = vec2[](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),

        vec2(1.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0) 
    );
    vec2 uv = uvs[gl_VertexIndex];
    outUV = uv;

    // 1. Calculate pixel bounds of the quad
    vec2 minPixel = offsetPixels; 
    vec2 maxPixel = offsetPixels + viewportExtent;

    // 2. Convert pixel bounds to Vulkan NDC (-1.0 to 1.0)
    vec2 minNDC = (minPixel / swapchainExtent) * 2.0 - 1.0;
    vec2 maxNDC = (maxPixel / swapchainExtent) * 2.0 - 1.0;

    // 3. Interpolate position for this vertex
    vec2 positionNDC = mix(minNDC, maxNDC, uv);

    gl_Position = vec4(positionNDC, 0.0, 1.0);
}