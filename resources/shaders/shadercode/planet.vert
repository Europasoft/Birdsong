#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout: require
// vertex inputs
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
// outputs to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPositionWS;
layout(location = 2) out vec3 fragNormalWS;
layout(location = 3) out vec2 fragUV;


// SCENE GLOBAL DESCRIPTOR SET
layout(std430, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
} ubo1;

// PUSH CONSTANTS (ShaderPushConstants::EngineMeshPushConstants)
layout(push_constant) uniform Push
{
	mat4 transform;
	mat4 normalMatrix;
} push;


// --- 3D simplex noise ---
vec3 mod289(vec3 x)
{
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x)
{
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
	return mod289(((x * 34.0) + 10.0) * x);
}

vec4 taylorInvSqrt(vec4 r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
{
	const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
	const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

	vec3 i = floor(v + dot(v, C.yyy));
	vec3 x0 = v - i + dot(i, C.xxx);

	vec3 g = step(x0.yzx, x0.xyz);
	vec3 l = 1.0 - g;
	vec3 i1 = min(g, l.zxy);
	vec3 i2 = max(g, l.zxy);

	vec3 x1 = x0 - i1 + C.xxx;
	vec3 x2 = x0 - i2 + C.yyy;
	vec3 x3 = x0 - D.yyy;

	i = mod289(i);

	vec4 p =
		permute(
		permute(
		permute(
			i.z + vec4(0.0, i1.z, i2.z, 1.0))
			+ i.y + vec4(0.0, i1.y, i2.y, 1.0))
			+ i.x + vec4(0.0, i1.x, i2.x, 1.0));

	float n_ = 1.0 / 7.0;
	vec3 ns = n_ * D.wyz - D.xzx;

	vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

	vec4 x_ = floor(j * ns.z);
	vec4 y_ = floor(j - 7.0 * x_);

	vec4 x = x_ * ns.x + ns.y;
	vec4 y = y_ * ns.x + ns.y;
	vec4 h = 1.0 - abs(x) - abs(y);

	vec4 b0 = vec4(x.xy, y.xy);
	vec4 b1 = vec4(x.zw, y.zw);

	vec4 s0 = floor(b0) * 2.0 + 1.0;
	vec4 s1 = floor(b1) * 2.0 + 1.0;
	vec4 sh = -step(h, vec4(0.0));

	vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
	vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

	vec3 p0 = vec3(a0.xy, h.x);
	vec3 p1 = vec3(a0.zw, h.y);
	vec3 p2 = vec3(a1.xy, h.z);
	vec3 p3 = vec3(a1.zw, h.w);

	vec4 norm = taylorInvSqrt(vec4(
		dot(p0, p0),
		dot(p1, p1),
		dot(p2, p2),
		dot(p3, p3)));

	p0 *= norm.x;
	p1 *= norm.y;
	p2 *= norm.z;
	p3 *= norm.w;

	vec4 m = max(0.6 - vec4(
		dot(x0, x0),
		dot(x1, x1),
		dot(x2, x2),
		dot(x3, x3)), 0.0);

	m *= m;

	return 42.0 * dot(
		m * m,
		vec4(
			dot(p0, x0),
			dot(p1, x1),
			dot(p2, x2),
			dot(p3, x3)));
}

// returns displacement height
float sineDisplacement(vec3 N, float freq, float amplitude)
{
	return sin(N.x * freq) * cos(N.y * freq) * amplitude;
}

// returns the gradient vector [dh/dx, dh/dy, dh/dz]
vec3 sineDisplacementGrad(vec3 N, float freq, float amplitude)
{
	float dx =  cos(N.x * freq) * freq * cos(N.y * freq) * amplitude;
	float dy = -sin(N.x * freq) * sin(N.y * freq) * freq * amplitude;
	float dz = 0.0;
	return vec3(dx, dy, dz);
}

float fbm(vec3 p, float frequency, float amplitude)
{
	float freq = frequency;
	float amp = amplitude;

	float value = 0.0;
	for (int i = 0; i < 5; ++i)
	{
		value += amp * snoise(p * freq);
		freq *= 2.0;
		amp *= 0.01;
	}

	return value;
}

// returns displacement height
float getTerrainHeight(vec3 p) 
{
	float midScaleMountains = fbm(p, 180.0, 0.0002);
	float largeHills = fbm(p, 2800.0, 0.00003);
	float midHills = fbm(p, 3500.0, 0.00003) * 1;
	float smallHills = fbm(p, 75000.0, 0.00001) * 1;
	return midScaleMountains + largeHills + midHills + smallHills;
}

vec3 calculateTerrainNormal(vec3 p)
{
	// Epsilon must scale relative to your noise frequency.
	// Small enough for fine details, large enough to avoid float precision issues.
	const float eps = 0.001; 

	// Sample terrain height at central point and 3 offset points along cartesian axes
	float h0 = getTerrainHeight(p);
	float hX = getTerrainHeight(normalize(p + vec3(eps, 0.0, 0.0)));
	float hY = getTerrainHeight(normalize(p + vec3(0.0, eps, 0.0)));
	float hZ = getTerrainHeight(normalize(p + vec3(0.0, 0.0, eps)));

	// Approximate partial derivatives (Gradient vector)
	vec3 grad = vec3(hX - h0, hY - h0, hZ - h0) / eps;

	// Project gradient onto the sphere's tangent plane to get slope relative to the surface
	vec3 tangentGrad = grad - p * dot(p, grad);

	// Subtract the projected gradient from the unperturbed normal
	return normalize(p - tangentGrad);
}

void main()
{
	vec3 sphereNormal = normalize(position);

	float h = getTerrainHeight(sphereNormal);
	vec3 displacedPos = sphereNormal * (1.0 + h);

	// output
	vec4 ws = push.transform * vec4(displacedPos, 1.0);

	gl_Position = ubo1.projectionViewMatrix * ws;
	fragPositionWS = ws.xyz;

	vec3 localNormal = calculateTerrainNormal(sphereNormal);
	fragNormalWS = normalize(mat3(push.normalMatrix) * localNormal); // normal to world space

	fragColor = mix(fragNormalWS, fragNormalWS, clamp(h, 0.0, 1.0));

	fragUV = uv;
}