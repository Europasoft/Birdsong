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

layout(push_constant) uniform Push
{
	vec4 uvs;
	vec4 vertexBounds;
	vec4 screenPos_FontScale_TexIdx; // xy = screen position, z = texture index, w = font scale
} push;

void main()
{
	float l = push.vertexBounds.x;
	float b = push.vertexBounds.y;
	float r = push.vertexBounds.z;
	float t = push.vertexBounds.w;

	float fontScale = push.screenPos_FontScale_TexIdx.z;

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

	vec2 offset = push.screenPos_FontScale_TexIdx.xy * 2.0 - 1.0;

	vec2 position =
		offset +
		vertices[gl_VertexIndex] *
		fontScale *
		ndcPerPixel;

	gl_Position = vec4(position, 0.0, 1.0);

	vec2 uvs[6] = vec2[]
	(
		vec2(push.uvs.x, push.uvs.y),
		vec2(push.uvs.z, push.uvs.y),
		vec2(push.uvs.z, push.uvs.w),

		vec2(push.uvs.x, push.uvs.y),
		vec2(push.uvs.z, push.uvs.w),
		vec2(push.uvs.x, push.uvs.w)
	);

	fragUV = uvs[gl_VertexIndex];
}