#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWS;
layout(location = 2) in vec3 fragNormalWS;

layout (location = 0) out vec4 outColor;
layout (depth_any) out float gl_FragDepth;

layout(set = 0, binding = 0) uniform GlobalFrameDataBuffer 
{
	mat4 projectionViewMatrix;
	vec4 ambientLightColor;
	vec3 lightPosition;
	vec4 lightColor;
} globalFrameData;

layout(push_constant) uniform Push	
{
	mat4 transform;
	mat4 normalMatrix;
} push;

void main() 
{
	vec3 dirToLight = globalFrameData.lightPosition - fragPositionWS;
	float attenuation = 1.0 / dot(dirToLight, dirToLight);

	vec3 lightColor = globalFrameData.lightColor.xyz * globalFrameData.lightColor.w * attenuation;
	vec3 lightAmbient = globalFrameData.ambientLightColor.xyz * globalFrameData.ambientLightColor.w;
	vec3 lightDiffuse = lightColor * max(dot(normalize(fragNormalWS), normalize(dirToLight)), 0);

	gl_FragDepth = 0.9;
	outColor = vec4((lightDiffuse + lightAmbient) * vec3(0.41,0.48,0.77), 1.0);
}