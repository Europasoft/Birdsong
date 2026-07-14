#version 450
#extension GL_EXT_scalar_block_layout: require
// input from vertex shader
layout(location = 0) in vec2 fragUV;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler _sampler;

layout(std430, set = 1, binding = 0) uniform UBO2
{
	vec2 extent;
} fx;

layout(set = 2, binding = 0) uniform texture2D attachment;

vec4 drawRectangle(vec2 res, vec2 pxPosIn, vec4 pxColorIn, float start, bool solid, float thickness) 
{
	float x = pxPosIn.x*res.x;
	float y = pxPosIn.y*res.y;
	float xmax = res.x - start;
	float ymax = res.y - start;
	bool a = (x > start && x < xmax) && (y > start && y < ymax);
	bool b = (x < start + thickness) || (x > xmax - thickness) || (y < start + thickness) ||(y > ymax - thickness);
	return mix(pxColorIn, vec4(1.0, 1.0, 1.0, 0.1), float(a && (solid || b)));
}

void main()
{
	vec2 resolution = vec2(1920, 1080);
	vec2 uv = fragUV; //gl_FragCoord.xy / resolution;
	outColor = texture(sampler2D(attachment, _sampler), uv);
	outColor = drawRectangle(resolution, uv, outColor, 8.0, false, 1.0);
	outColor = drawRectangle(resolution, uv, outColor, 12.0, false, 1.0);
	gl_FragDepth = 0.99999;
}