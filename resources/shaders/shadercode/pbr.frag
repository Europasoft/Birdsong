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

struct InstanceData 
{
    mat4 modelMatrix;
    mat4 normalMatrix;
    uint albedoTexIdx;
    uint normalTexIdx;
    uint roughnessTexIdx;
    uint _pad;
};

layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
    mat4 normalMatrix;
} ubo1;

layout(set = 0, binding = 1) uniform texture2D textures[2];
layout(set = 0, binding = 2) uniform sampler _sampler;

// second descriptor set: unbounded texture array
layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

layout(buffer_reference, scalar) readonly buffer InstanceBufferRef 
{
    InstanceData instances[];
};

layout(push_constant) uniform PushConstants 
{
    InstanceBufferRef instanceBuffer;
    uint instanceID;
} push;

#define PI 3.1415926535897932384626433832795

vec3 Fresnel(vec3 H, vec3 V)
{
    vec3 F0 = vec3(0.03, 0.03, 0.03); // base reflectivity
    float HdotV = max(dot(H, V), 0.0);
    return F0 + (1.0-F0) * pow(1-HdotV, 5.0);
}

float TrowbridgeReitzNDF(vec3 N, vec3 H, float a)
{
    float a2 = a*a;
    float NH2 = max(dot(N, H), 0.0) * max(dot(N, H), 0.0);
    float d = PI * ((NH2 * (a2-1.0) + 1.0) * (NH2 * (a2-1.0) + 1.0));
    return a2 / d;
}

float SchlickGeometry(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0-k) + k);
}
  
float SmithGeometry(vec3 N, vec3 V, vec3 L, float k)
{
    float a = SchlickGeometry(max(dot(N, V), 0.0), k);
    float b = SchlickGeometry(max(dot(N, L), 0.0), k);
    return a * b;
}

vec3 BRDF(vec3 baseColor, vec3 N, vec3 V, vec3 L, vec3 H, float roughness)
{
    float k = (roughness+1)*(roughness+1);
    float NDF = TrowbridgeReitzNDF(N, H, roughness);
    float G = SmithGeometry(N, V, L, k);
    vec3 Specular = Fresnel(H, L) * G * NDF / (4*dot(V,N)*dot(L,N)); 
    vec3 Lambertian = baseColor / PI;
	vec3 Diffuse = ((1-Fresnel(N, L)) * (1-Fresnel(N, V))) * Lambertian;
    return Specular + Diffuse;
}

void main()
{
    vec3 lightPos = vec3(10.0, 10.0, 10.0); // temporary
    vec3 camPos = vec3(0.0, 0.0, 0.0); // temporary
    vec3 lightDir = normalize(lightPos - fragPositionWS);
    vec3 viewDir = normalize(camPos - fragPositionWS);
    vec3 halfwayVec = normalize(lightDir + viewDir);

    float effectiveRoughness = 0.5; // temporary
    float indirect = 0.001;
    // nonuniformEXT tells driver that different threads inside the warp/wavefront might sample different texture indices simultaneously
    vec4 baseColor = texture(globalTextures[nonuniformEXT(1000)], fragUV);
	vec3 litColor = BRDF(baseColor.xyz, normalize(fragNormalWS), viewDir, lightDir, halfwayVec, effectiveRoughness);
    outColor = vec4(litColor.x, litColor.y, litColor.z, baseColor.w) + indirect;
}