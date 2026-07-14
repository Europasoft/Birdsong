#version 450
#extension GL_EXT_scalar_block_layout: require
// no vertex inputs, vertices are generated here
layout(location = 0) out vec2 fragUV; // texture coordinate output

layout(push_constant) uniform Push
{
  vec2 position;
	vec2 size;
  float timeSinceHover;
  float timeSinceClick;
} push;

void main() 
{
    float xs = push.size.x;
    float ys = push.size.y;
    vec2 vertices[6] = vec2[]
    (
      vec2(0.0, ys), vec2(0.0, 0.0), vec2(xs, 0.0), 
      vec2(xs, ys), vec2(0.0, ys), vec2(xs, 0.0)
    );

    vec2 screenPosition = ((push.position*2.0)-1.0) + vertices[gl_VertexIndex];
    gl_Position = vec4(screenPosition, 0.0, 1.0);
    fragUV = screenPosition;
}