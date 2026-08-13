#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout: require
// input from vertex shader
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec2 boxSize; // size in pixels
layout(location = 3) in vec4 boxCornerRadii;

layout (location = 0) out vec4 outColor;

// SCENE GLOBAL DESCRIPTOR SET
layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
    vec2 viewportExtent;
} ubo1;

// second scene-global descriptor set: unbounded texture array
layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

float sdRoundedBox(vec2 p, vec2 b, vec4 r) 
{
    r.xy = (p.x > 0.0) ? r.yw : r.xz;
    r.x  = (p.y > 0.0) ? r.y  : r.x;
    
    vec2 q = abs(p) - b + vec2(r.x);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

vec4 roundCorners(vec4 color)
{
	// convert UV [0,1] to pixel coordinates centered at (0,0)
    vec2 pixelPos = (fragUV - 0.5) * boxSize;
    vec2 halfSize = boxSize * 0.5;

    // calculate distance to the box edge
    float d = sdRoundedBox(pixelPos, halfSize, boxCornerRadii);

    // smooth anti-aliasing using fwidth()
    float smoothness = fwidth(d);
    float alpha = smoothstep(0.0, -smoothness, d);
	alpha = clamp(alpha, 0.0, 1.0);

    // clip pixels outside the rounded shape
    return vec4(color.rgb, color.a * alpha);
}

void main()
{
	outColor = roundCorners(vertexColor);
}