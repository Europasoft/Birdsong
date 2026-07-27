#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
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


//struct testStruct
//{
//	float s;
//	vec3 v;
//};

layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
  //testStruct[2] test;
} ubo1;

layout(set = 0, binding = 1) uniform texture2D textures[2];
layout(set = 0, binding = 2) uniform sampler _sampler;

struct InstanceData 
{
    mat4 modelMatrix;
	mat4 normalMatrix;
    uint albedoTexIdx;
    uint normalTexIdx;
    uint roughnessTexIdx;
    uint _pad;
};

layout(buffer_reference, scalar) readonly buffer InstanceBufferRef 
{
    InstanceData instances[];
};

layout(push_constant) uniform PushConstants 
{
    InstanceBufferRef instanceBuffer;
    uint instanceID;
} push;

mat4 blenderToVulkan1()
{
	mat4 m = mat4(0.0);
	m[2][0] = 1.f;
	m[1][1] = 1.f;
	m[0][2] = 1.f;
	m[3][3] = 1.f;
	return m;
}
mat4 blenderToVulkan2()
{
	mat4 m = mat4(0.0);
	m[0][0] = 1.f;
	m[2][1] = -1.f;
	m[1][2] = -1.f;
	m[3][3] = 1.f;
	return m;
}

void main()
{
	InstanceData instance = push.instanceBuffer.instances[push.instanceID];
    mat4 modelMat = instance.modelMatrix;
	mat4 normalMat = instance.normalMatrix;

	gl_Position =  ubo1.projectionViewMatrix * modelMat * position;
	fragNormalWS = normalize(mat3(normalMat) * normal);
	fragPositionWS = vec4( modelMat * position).xyz;
	fragUV = uv;
	fragColor = color;
}