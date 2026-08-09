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

// layout of an instance in the glyph instance buffer
struct GlyphInstanceData 
{
	vec4 uvs;
	vec4 vertexBounds;
	vec2 basePos;
	float fontScale;
	uint textureIndex;
};

// glypth instance buffer passed by BDA
layout(buffer_reference, scalar) readonly buffer GlyphInstanceBufferRef 
{
    GlyphInstanceData instances[];
};

// standard push constant (ShaderPushConstants::MeshPushConstants)
layout(push_constant) uniform PushConstants 
{
    GlyphInstanceBufferRef glyphInstanceBuffer;
    uint instanceID;
} push;

float median(float r, float g, float b)
{
	return max(min(r, g), min(max(r, g), b));
}

void main()
{
	GlyphInstanceData instance = push.glyphInstanceBuffer.instances[push.instanceID];

	vec4 s = texture(globalTextures[nonuniformEXT(instance.textureIndex)], fragUV);
	float sd = median(s.r, s.g, s.b);
	float opacity = clamp((sd - 0.5) * 10.0 + 0.5, 0.0, 1.0);
	outColor = vec4(1.0, 1.0, 1.0, opacity);
}