#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout: require
// input from vertex shader
layout(location = 0) in vec2 fragUV;

layout (location = 0) out vec4 outColor;

// second scene-global descriptor set: unbounded texture array
layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

layout(push_constant) uniform Push
{
  vec4 uvs;
  vec4 vertexBounds;
  vec4 screenPositionAndTextureIndex;
} push;

void main()
{
	//outColor = vec4(1,1,1,1);
	outColor = texture(globalTextures[nonuniformEXT(2)], fragUV);
}