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
//layout(set = 0, binding = 1) uniform sampler2D texSampler;


void main()
{
	//vec3 dirToLight = globalFrameData.lightPosition - fragPositionWS;
	//float attenuation = 1.0 / dot(dirToLight, dirToLight);

	//vec3 lightColor = globalFrameData.lightColor.xyz * globalFrameData.lightColor.w * attenuation;
	//vec3 lightAmbient = globalFrameData.ambientLightColor.xyz * globalFrameData.ambientLightColor.w;
	//vec3 lightDiffuse = lightColor * max(dot(normalize(fragNormalWS), normalize(dirToLight)), 0);

	//outColor = vec4((lightDiffuse + lightAmbient) * fragColor, 1.0);
	//outColor = vec4(fragUV.x * ubo2.hue.x, fragUV.y * ubo2.hue.y, 0.5 * ubo2.hue.z, 1.0); // test uv coords
	//outColor = texture(texSampler, fragUV);
	//outColor = vec4(t.x * ubo2.scalars[0], t.y * ubo2.scalars[1], t.z * ubo2.scalars[2], t.w);
	outColor = texture(sampler2D(textures[0], _sampler), fragUV);
}