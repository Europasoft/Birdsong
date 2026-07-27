#version 450
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

// descriptor set 0 inputs
layout(std140, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
	vec3 cameraPosition;
} ubo1;

layout(push_constant) uniform Push
{
	mat4 transform;
	mat4 normalMatrix;
} push;
//globalFrameData.projectionViewMatrix
void main()
{
  gl_Position = ubo1.projectionViewMatrix * push.transform * position;
  fragNormalWS = normalize(mat3(push.normalMatrix) * normal);
  fragPositionWS = vec4(push.transform * position).xyz;
  fragColor = vec3(0.8, 0.6, 0.6); // use fixed value instead of vertex color
  fragUV = uv;
}