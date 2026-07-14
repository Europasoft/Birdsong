#version 450
#extension GL_EXT_scalar_block_layout: require
// no vertex inputs, vertices are generated here
layout(location = 0) out vec2 fragUV; // texture coordinate output

void main() 
{
    //create full-screen triangle
    fragUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(fragUV * 2.0f + -1.0f, 0.0f, 1.0f);

    //vec2 vertices[3] = vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));
    //gl_Position = vec4(vertices[gl_VertexIndex], 0, 1);
    //fragUV = 0.5 * gl_Position.xy + vec2(0.5);
}