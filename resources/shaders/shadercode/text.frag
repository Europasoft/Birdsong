#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout: require
// input from vertex shader
layout(location = 0) in vec2 fragUV;

layout (location = 0) out vec4 outColor;

// scene-global descriptor set 0 is available but not needed here

// second scene-global descriptor set: unbounded texture array
layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

// layout of an instance in the glyph instance buffer
struct GlyphInstanceData 
{
	vec4 uvs;
	vec4 vertexBounds;
	vec4 basePosFontScaleAndTextureIndex; // xy = basePos, z = fontScale, w = textureIndex
	vec4 targetAttachmentResolution; // may be drawing to a smaller viewport or the full swapchain image
};

// glypth instance buffer passed by BDA
layout(buffer_reference, scalar) readonly buffer GlyphInstanceBufferRef 
{
    GlyphInstanceData instances[];
};

// standard push constant (ShaderPushConstants::MeshPushConstants)
layout(push_constant) uniform PushConstants 
{
    GlyphInstanceBufferRef glyphInstanceBuffer;
    uint instanceID;
} push;

float median(float r, float g, float b)
{
	return max(min(r, g), min(max(r, g), b));
}

float sdfOpacity(GlyphInstanceData instance)
{
    uint textureIndex = floatBitsToUint(instance.basePosFontScaleAndTextureIndex.w);

	vec4 s = texture(globalTextures[nonuniformEXT(textureIndex)], fragUV); // sample SDF atlas texture
    float sd = median(s.r, s.g, s.b) - 0.5; // raw distance centred at 0

    // texture size in pixels
    vec2 atlasSize = vec2(textureSize(globalTextures[nonuniformEXT(textureIndex)], 0));

    // screen-space scale factor, converts texels to screen pixels
    float pxRange = 9.0; // should match the sdfPixelRange used during atlas generation (see Fonts.h)
    vec2 unitSize = vec2(pxRange) / atlasSize;
    vec2 screenTexSize = vec2(1.0) / fwidth(fragUV);
    float screenPxRange = max(0.5 * dot(unitSize, screenTexSize), 1.0);

    // distance in screen pixels
    float screenPxDistance = sd * screenPxRange;

    // linear 1-pixel anti-aliased edge
    return clamp(screenPxDistance + 0.5, 0.0, 1.0);
}

void main()
{
	GlyphInstanceData instance = push.glyphInstanceBuffer.instances[push.instanceID];

    float opacity = sdfOpacity(instance);

    outColor = vec4(1.0, 1.0, 1.0, opacity);
}
