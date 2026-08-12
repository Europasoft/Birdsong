#include "core/ui/TextBox.h"
#include "core/ui/Fonts.h"
#include "core/gpu/Material.h"
#include "core/draw/FrameContext.h"
#include "core/gpu/descriptors/InstanceBuffer.h"

#include "core/types/vk.h"

namespace UI
{
	TextBox::TextBox()
	{
	}

	TextBox::~TextBox()
	{
	}

	void TextBox::setFont(std::shared_ptr<Font> newFont)
	{
		font = newFont;
	}

	void TextBox::loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d)
	{
		HorizontalBox::loadMaterial(device, d);

		// text needs a material with special shaders
		using namespace EngineCore;
		ShaderFilePaths shaderPaths(makePath("shaders/compiled/text.vert.spv"), makePath("shaders/compiled/text.frag.spv"));
		MaterialCreateInfo textMatInfo(shaderPaths, d.world->getScene().getDescriptorSetLayouts(), d.samples, d.basePassFormats,
			sizeof(ShaderPushConstants::MeshPushConstants));
		textMatInfo.shadingProperties.useVertexInput = false;
		textMatInfo.shadingProperties.enableDepth = false;
		textMatInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		textMaterial = std::make_shared<Material>(textMatInfo, device);
		textMaterial->finalize();
	}

	void TextBox::preDraw(const EngineCore::FrameContext& f, const PreDrawData& data)
	{
		HorizontalBox::preDraw(f, data);

		if (not (text.size() && font && textMaterial)) return;

		// add text info to glyph instance buffer
		float offset = 0;
		Vec2 basePosition = Element::calculatePosition(data.size);
		basePosition.y += size.y * (parent ? parent->size.y : 0.f); // make text rest on the bottom edge of the element
		for (size_t i = 0; i < text.size(); i++)
		{
			const GlyphInfo& g = font->getCharacter(text[i]);
			const uint32_t glyphInstanceID = addGlyphToInstanceBuffer(g, offset, basePosition);
			firstGlyphInstanceBufferID = (i == 0) ? glyphInstanceID : firstGlyphInstanceBufferID;
			const auto kerning = (i == text.size() - 1) ? 0.f : font->getKerning(text[i], text[i + 1]);
			float addOffset = (g.advance + kerning) * fontScale / f.viewportExtent.width; // convert (advance + kerning) to normalized [0, 1] width
			offset += addOffset;
		}
	}

	void TextBox::draw(const EngineCore::FrameContext & f, EngineCore::Material * &m)
	{
		// draw background behind the text
		HorizontalBox::draw(f, m);

		if (not (text.size() && font && textMaterial)) return;

		// draw text
		const auto globalSets = f.scene->getDescriptorSets(f.bufferIndex);
		vkCmdBindDescriptorSets(f.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, textMaterial->getPipelineLayout(),
			0, static_cast<uint32_t>(globalSets.size()), globalSets.data(), 0, nullptr);

		textMaterial->bindToCommandBuffer(f.commandBuffer);

		const auto bda = root->getTextGlyphInstanceBuffer().getDeviceAddress(f.bufferIndex);
		EngineCore::ShaderPushConstants::MeshPushConstants push = {};
		for (size_t i = 0; i < text.size(); i++)
		{
			push.instanceBufferAddress = bda;
			push.instanceID = firstGlyphInstanceBufferID + i;
			textMaterial->writePushConstants(f.commandBuffer, push);
			vkCmdDraw(f.commandBuffer, 6, 1, 0, 0); // bufferless draw (vertex attributes generated in shader)
		}
	}

	uint32_t TextBox::addGlyphToInstanceBuffer(const UI::GlyphInfo& g, float offset, const Vec2& basePosition)
	{
		EngineCore::ShaderInstanceData::TextGlyphInstanceData d = {};
		d.uvs.x = g.u0;
		d.uvs.y = g.v0;
		d.uvs.z = g.u1;
		d.uvs.w = g.v1;
		d.vertexBounds.x = g.l;
		d.vertexBounds.y = g.b;
		d.vertexBounds.z = g.r;
		d.vertexBounds.w = g.t;
		d.basePos.x = basePosition.x + offset;
		d.basePos.y = basePosition.y;
		d.fontScale = fontScale;
		d.textureIndex = font->getTextureIndex();
		return root->getTextGlyphInstanceBuffer().addInstanceData(d);
	}

}