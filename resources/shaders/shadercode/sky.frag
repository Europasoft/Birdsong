#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout: require
// inputs from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWS;
layout(location = 2) in vec3 fragNormalWS;
layout(location = 3) in vec2 fragUV;

layout (location = 0) out vec4 outColor;
layout (depth_any) out float gl_FragDepth;

// second descriptor set: unbounded texture array
layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

void main() 
{
	gl_FragDepth = 0.9999999;
	// assuming the sky texture is at index 1 in the array
	vec4 baseColor = texture(globalTextures[1], fragUV);
	outColor = baseColor;
}