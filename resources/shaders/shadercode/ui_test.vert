#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout: require
// no vertex inputs, vertices are generated here
layout(location = 0) out vec2 fragUV; // texture coordinate output
layout(location = 1) out vec4 vertexColor;

// SCENE GLOBAL DESCRIPTOR SET
layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
    vec2 viewportExtent;
} ubo1;

// second scene-global descriptor set: unbounded texture array
layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

// layout of an instance in the UI element instance buffer
struct UIInstanceData 
{
	vec4 positionAndSize;
	vec4 backgroundColor;
};

// UI element instance buffer passed by BDA
layout(buffer_reference, scalar) readonly buffer UIInstanceBufferRef 
{
	UIInstanceData instances[];
};

// standard push constant (ShaderPushConstants::MeshPushConstants)
layout(push_constant) uniform PushConstants 
{
	UIInstanceBufferRef uiInstanceBuffer;
	uint instanceID;
} push;

void main() 
{
	UIInstanceData instance = push.uiInstanceBuffer.instances[push.instanceID];
	vec2 position = instance.positionAndSize.xy;
	vec2 size = instance.positionAndSize.zw;
	
	vec2 p = position * 2.0 - 1.0;
	vec2 s = size * 2.0;

	vec2 vertex = vec2[]
	(
		vec2(0.0, s.y),
		vec2(0.0, 0.0),
		vec2(s.x, 0.0),
		vec2(s.x, s.y),
		vec2(0.0, s.y),
		vec2(s.x, 0.0)
	)[gl_VertexIndex];

	gl_Position = vec4(p + vertex, 0.0, 1.0);

	vertexColor = instance.backgroundColor;
	fragUV = p;
}