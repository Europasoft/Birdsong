#include "core/ui/TextBox.h"
#include "core/ui/Fonts.h"
#include "core/gpu/Material.h"
#include "core/draw/FrameContext.h"
#include "core/gpu/descriptors/InstanceBuffer.h"

#include "core/types/vk.h"

#include <algorithm>
#include <limits>
#include <cmath>
#include <cstdlib>

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
		Element::loadMaterial(device, d);

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
		Element::preDraw(f, data);

		if (not (text.size() && font && textMaterial)) return;

		Vec2 textPosition = alignText((data.position - data.pivot), f, data);
	
		// add text info to glyph instance buffer
		float offset = 0;
		for (size_t i = 0; i < text.size(); i++)
		{
			const GlyphInfo& g = font->getCharacter(text[i]);
			const uint32_t glyphInstanceID = addGlyphToInstanceBuffer(g, offset, textPosition);
			firstGlyphInstanceBufferID = (i == 0) ? glyphInstanceID : firstGlyphInstanceBufferID;
			offset += getAdvanceForChar(i, g, f);
		}
	}

	void TextBox::draw(const EngineCore::FrameContext& f, EngineCore::Material * &m)
	{
		// draw background behind the text
		Element::draw(f, m);

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

	Vec2 TextBox::alignText(Vec2 textPosition, const EngineCore::FrameContext& f, const PreDrawData& data)
	{
		float maxTop = -std::numeric_limits<float>::infinity();
		float minBottom = std::numeric_limits<float>::infinity();
		float normalizedWidth = 0.f;
		if (alignVertical != EAlignV::BOTTOM || alignHorizontal != EAlignH::LEFT)
		{
			for (size_t i = 0; i < text.size(); i++)
			{
				const GlyphInfo& g = font->getCharacter(text[i]);
				maxTop = std::max(maxTop, g.t);
				minBottom = std::min(minBottom, g.b);
				normalizedWidth += getAdvanceForChar(i, g, f);
			}
		}

		// vertical alignment
		if (alignVertical == EAlignV::BOTTOM)
		{
			textPosition += data.size * Vec2(0, 1); // text rests on element's bottom edge
		}
		else if (alignVertical == EAlignV::CENTER)
		{
			// find the midpoint of the text's bounding box relative to the baseline
			const float textMidpointPixels = (maxTop + minBottom) * 0.5f;
			// pixel offset to normalized screen space
			const float normalizedMidpoint = (textMidpointPixels * fontScale) / f.viewportExtent.height;
			// position baseline so the text's bounding box center sits at containerCenterY
			const float containerCenterY = data.position.y + (data.size.y * 0.5f);
			textPosition.y = containerCenterY + normalizedMidpoint;
		}
		else if (alignVertical == EAlignV::TOP)
		{
			textPosition.y = data.position.y + (maxTop * fontScale) / f.viewportExtent.height;
		}

		// horizontal alignment
		if (alignHorizontal == EAlignH::CENTER)
		{
			const float containerCenterX = data.position.x + (data.size.x * 0.5f);
			textPosition.x = containerCenterX - (normalizedWidth * 0.5f);
		}
		else if (alignHorizontal == EAlignH::RIGHT)
		{
			const float containerEndX = data.position.x + data.size.x ;
			textPosition.x = containerEndX - normalizedWidth;
		}

		
		return textPosition;
	}

	float TextBox::getAdvanceForChar(size_t i, const GlyphInfo& g, const EngineCore::FrameContext& f) const
	{
		const float kerning = (i == text.size() - 1) ? 0.f : font->getKerning(text[i], text[i + 1]);
		// convert (advance + kerning) to normalized [0, 1] width
		return (g.advance + kerning) * fontScale / f.viewportExtent.width; 
	}


}