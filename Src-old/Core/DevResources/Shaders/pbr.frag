#version 450
#extension GL_EXT_scalar_block_layout: require
// inputs from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWS;
layout(location = 2) in vec3 fragNormalWS;
layout(location = 3) in vec2 fragUV;

layout (location = 0) out vec4 outColor;


layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
} ubo1;

layout(set = 0, binding = 1) uniform texture2D textures[2];
layout(set = 0, binding = 2) uniform sampler _sampler;

layout(std430, set = 1, binding = 0) uniform UBO2 
{
	vec3 cameraPosition;
    vec3 lightPosition;
    float roughness;
} ubo2;

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
    vec3 lightDir = normalize(ubo2.lightPosition - fragPositionWS);
    vec3 viewDir = normalize(ubo2.cameraPosition - fragPositionWS);
    vec3 halfwayVec = normalize(lightDir + viewDir);

    float effectiveRoughness = ubo2.roughness;
    float indirect = 0.001;
    //vec4 baseColor = texture(sampler2D(textures[0], _sampler), fragUV);
    float colorGrayscale = 1.0;
    vec4 baseColor = vec4(colorGrayscale,colorGrayscale,colorGrayscale,1.0);
	vec3 litColor = BRDF(baseColor.xyz, normalize(fragNormalWS), viewDir, lightDir, halfwayVec, effectiveRoughness);
    outColor = vec4(litColor.x, litColor.y, litColor.z, baseColor.w) + indirect;
    //outColor = vec4(fragNormalWS.x, fragNormalWS.y, fragNormalWS.z, 1.0);
}