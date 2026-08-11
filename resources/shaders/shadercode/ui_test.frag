#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout: require
// input from vertex shader
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 vertexColor;

layout (location = 0) out vec4 outColor;

// SCENE GLOBAL DESCRIPTOR SET
layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
    vec2 viewportExtent;
} ubo1;

// second scene-global descriptor set: unbounded texture array
layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

void main()
{
	outColor = vertexColor;
}