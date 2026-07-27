#version 450
#extension GL_EXT_scalar_block_layout: require
// inputs from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWS;
layout(location = 2) in vec3 fragNormalWS;
layout(location = 3) in vec2 fragUV;

layout (location = 0) out vec4 outColor;


layout(set = 0, binding = 2) uniform sampler _sampler;

layout(std430, set = 1, binding = 0) uniform UBO2
{
	vec2 extent;
} fx;

// rendered image from previous pass
layout(set = 2, binding = 0) uniform texture2D attachment;


void main()
{
	vec2 resolution = vec2(1920, 1080); // hardcoded resolution!


	float Pi = 6.28318530718; // Pi*2
    float Directions = 16.0; // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
    float Quality = 6.0; // BLUR QUALITY (Default 4.0 - More is better but slower)
    float Size = 7.0; // BLUR SIZE (Radius)
    vec2 Radius = Size/resolution;
	vec2 uv = gl_FragCoord.xy / resolution;
	uv = gl_FragCoord.xy / resolution + fragNormalWS.xy / 20.0;
	vec4 color = texture(sampler2D(attachment, _sampler), uv);
	for(float d=0.0; d<Pi; d+=Pi/Directions)
    {
		for(float i=1.0/Quality; i<=1.0; i+=1.0/Quality)
        {
			color += texture(sampler2D(attachment, _sampler), uv+vec2(cos(d),sin(d))*Radius*i);
        }
    }

	color /= Quality * Directions - 15.0;
	outColor = color + (vec4(0.04, 0.07, 0.1, 1.0)/2.0);
}