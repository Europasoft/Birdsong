#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout: require
// no vertex inputs, vertices are generated here

layout(location = 0) out vec2 fragUV; // texture coordinate output

// SCENE GLOBAL DESCRIPTOR SET
layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
    vec2 viewportExtent;
} ubo1;

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

void main()
{
	GlyphInstanceData instance = push.glyphInstanceBuffer.instances[push.instanceID];

	float l = instance.vertexBounds.x;
	float b = instance.vertexBounds.y;
	float r = instance.vertexBounds.z;
	float t = instance.vertexBounds.w;

	vec2 ndcPerPixel = 2.0 / ubo1.viewportExtent;

	vec2 vertices[6] = vec2[]
	(
		vec2(l, b),
		vec2(r, b),
		vec2(r, t),

		vec2(l, b),
		vec2(r, t),
		vec2(l, t)
	);

	vec2 position =
		instance.basePos
		+ vertices[gl_VertexIndex] * vec2(1.0, -1.0) /* y flipped */
		* instance.fontScale * ndcPerPixel;

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