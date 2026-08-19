#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout: require
// no vertex inputs, vertices are generated here

layout(location = 0) out vec2 fragUV; // texture coordinate output

// scene-global descriptor set 0 is available but not needed here

// second scene-global descriptor set: unbounded texture array
layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

// layout of an instance in the glyph instance buffer
struct GlyphInstanceData 
{
	vec4 uvs;
	vec4 vertexBounds;
	vec4 basePosFontScaleAndTextureIndex; // xy = basePos, z = fontScale, w = textureIndex
	vec4 targetAttachmentResolution; // may be drawing to a smaller viewport or the full swapchain image
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

void main()
{
	GlyphInstanceData instance = push.glyphInstanceBuffer.instances[push.instanceID];
	vec2 basePos = instance.basePosFontScaleAndTextureIndex.xy;
	float fontScale = instance.basePosFontScaleAndTextureIndex.z;
	uint textureIndex = floatBitsToUint(instance.basePosFontScaleAndTextureIndex.w);
	vec2 attachmentResolution = instance.targetAttachmentResolution.xy;

	float l = instance.vertexBounds.x;
	float b = instance.vertexBounds.y;
	float r = instance.vertexBounds.z;
	float t = instance.vertexBounds.w;

	vec2 ndcPerPixel = 2.0 / attachmentResolution;

	vec2 vertices[6] = vec2[]
	(
		vec2(l, b),
		vec2(r, b),
		vec2(r, t),

		vec2(l, b),
		vec2(r, t),
		vec2(l, t)
	);

	// convert basePos from [0..1] normalized space to [-1..1] NDC space
    vec2 p = basePos * 2.0 - 1.0;
    // add pixel offset converted to NDC space
    vec2 position = p + vertices[gl_VertexIndex] * vec2(1.0, -1.0) * fontScale * ndcPerPixel;

	gl_Position = vec4(position, 0.0, 1.0);

	vec2 uvs[6] = vec2[]
	(
		vec2(instance.uvs.x, instance.uvs.w),
		vec2(instance.uvs.z, instance.uvs.w),
		vec2(instance.uvs.z, instance.uvs.y),

		vec2(instance.uvs.x, instance.uvs.w),
		vec2(instance.uvs.z, instance.uvs.y),
		vec2(instance.uvs.x, instance.uvs.y)
	);

	fragUV = uvs[gl_VertexIndex];
}