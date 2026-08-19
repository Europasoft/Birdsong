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

// SCENE GLOBAL DESCRIPTOR SET
layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
  vec4 resolution; // viewport and swapchain resolutions in pixels
} ubo1;
// FX DESCRIPTOR SET - ATTACHMENT FROM PREVIOUS PASS
layout(set = 1, binding = 0) uniform texture2D attachment;
// FX DESCRIPTOR SET - SAMPLER
layout(set = 2, binding = 0) uniform sampler _attachmentSampler;

// PUSH CONSTANTS - SPECIFIC TO FX PASS
layout(push_constant) uniform Push
{
	mat4 transform;
	mat4 normalMatrix;
} push;


void main()
{
  gl_Position = ubo1.projectionViewMatrix * push.transform * position;
  fragNormalWS = normalize(mat3(push.normalMatrix) * normal);
  fragPositionWS = vec4(push.transform * position).xyz;
  fragColor = vec3(0.8, 0.6, 0.6); // use fixed value instead of vertex color
  fragUV = uv;
}