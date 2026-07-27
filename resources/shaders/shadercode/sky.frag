#version 450
#extension GL_EXT_scalar_block_layout: require
// inputs from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWS;
layout(location = 2) in vec3 fragNormalWS;
layout(location = 3) in vec2 fragUV;

layout (location = 0) out vec4 outColor;
layout (depth_any) out float gl_FragDepth;


layout(set = 0, binding = 1) uniform texture2D textures[2];
layout(set = 0, binding = 2) uniform sampler _sampler;


void main() 
{
	gl_FragDepth = 0.9999999;
	outColor = texture(sampler2D(textures[1], _sampler), fragUV);
}