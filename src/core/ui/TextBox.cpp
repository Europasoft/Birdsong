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
#include <cctype>
#include <bit>

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

	void TextBox::preDraw(const EngineCore::FrameContext& f, const PreDrawData& data, Vec2 renderPosition)
	{
		Element::preDraw(f, data, renderPosition);

		if (not (text.size() && font && textMaterial)) return;

		const ScaledFontMetrics metrics = font->getScaledMetrics(f.viewport.extent.y, fontScale);
		std::vector<Line> lines = processLines(f, data);
		generateInstances(lines, metrics, f, data);
	}

	std::vector<Line> TextBox::processLines(const EngineCore::FrameContext& f, const PreDrawData& data) const
	{
		std::vector<Line> lines;
		size_t currentLineStart = 0;
		float currentLineWidth = 0.f;

		size_t lastSpaceIndex = std::string::npos;
		float widthAtLastSpace = 0.f;

		for (size_t i = 0; i < text.size(); i++)
		{
			const GlyphInfo& g = font->getCharacter(text[i]);
			const float charAdvance = getAdvanceForChar(i, g, f);

			if (std::isspace(text[i]))
			{
				lastSpaceIndex = i;
				widthAtLastSpace = currentLineWidth;
			}

			// check if adding this character exceeds container width
			if (currentLineWidth + charAdvance > data.size.x && i > currentLineStart)
			{
				if (lastSpaceIndex != std::string::npos && lastSpaceIndex >= currentLineStart)
				{
					// break line at the last space
					size_t lineLength = lastSpaceIndex - currentLineStart;
					lines.push_back({ currentLineStart, lineLength, widthAtLastSpace });

					// resume from character after the space
					currentLineStart = lastSpaceIndex + 1;

					// recalculate width of the current word being carried over
					currentLineWidth = 0.0f;
					for (size_t j = currentLineStart; j <= i; j++)
					{
						const GlyphInfo& wG = font->getCharacter(text[j]);
						currentLineWidth += getAdvanceForChar(j, wG, f);
					}
				}
				else
				{
					// single word exceeds container width: hard break at current character
					lines.push_back({ currentLineStart, i - currentLineStart, currentLineWidth });
					currentLineStart = i;
					currentLineWidth = charAdvance;
				}
				lastSpaceIndex = std::string::npos;
			}
			else
			{
				currentLineWidth += charAdvance;
			}
		}

		// push final remaining line
		if (currentLineStart < text.size())
		{
			lines.push_back({ currentLineStart, text.size() - currentLineStart, currentLineWidth });
		}

		return lines;
	}

	void TextBox::generateInstances(const std::vector<Line>& lines, const ScaledFontMetrics& metrics,
		const EngineCore::FrameContext& f, const PreDrawData& data)
	{
		// calculate vertical alignment for the entire block of lines
		const float totalTextHeight = lines.size() * metrics.lineHeight; // line height normalized [0, 1]
		Vec2 blockPosition = data.position - data.pivot;
		if (alignVertical == EAlignV::CENTER)
		{
			blockPosition.y += (data.size.y - totalTextHeight) * 0.5f;
		}
		else if (alignVertical == EAlignV::BOTTOM)
		{
			blockPosition.y += data.size.y - totalTextHeight;
		}

		// shift block position down by the ascender so line 0's baseline 
		// is positioned correctly relative to the top of the text block
		blockPosition.y += metrics.ascender;

		bool isFirstGlyph = true;

		for (size_t lineIdx = 0; lineIdx < lines.size(); lineIdx++)
		{
			const auto& line = lines[lineIdx];

			// calculate horizontal starting position per line
			Vec2 linePos = blockPosition;
			linePos.y += lineIdx * metrics.lineHeight;

			if (alignHorizontal == EAlignH::CENTER)
			{
				linePos.x += (data.size.x - line.width) * 0.5f;
			}
			else if (alignHorizontal == EAlignH::RIGHT)
			{
				linePos.x += data.size.x - line.width;
			}

			float xOffset = 0.0f;
			for (size_t i = line.startIndex; i < line.startIndex + line.length; i++)
			{
				// add instance data for glyph to storage buffer
				const GlyphInfo& g = font->getCharacter(text[i]);
				const uint32_t glyphInstanceID = addGlyphToInstanceBuffer(g, xOffset, linePos, f);

				if (isFirstGlyph)
				{
					firstGlyphInstanceBufferID = glyphInstanceID;
					isFirstGlyph = false;
				}

				xOffset += getAdvanceForChar(i, g, f);
			}
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

	uint32_t TextBox::addGlyphToInstanceBuffer(const UI::GlyphInfo& g, float offset, const Vec2& basePosition, const EngineCore::FrameContext& f)
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
		d.basePosFontScaleAndTextureIndex.x = basePosition.x + offset;
		d.basePosFontScaleAndTextureIndex.y = basePosition.y;
		d.basePosFontScaleAndTextureIndex.z = fontScale;
		d.basePosFontScaleAndTextureIndex.w = std::bit_cast<float>(font->getTextureIndex());
		d.targetAttachmentResolution.x = f.viewport.extent.x;
		d.targetAttachmentResolution.y = f.viewport.extent.y;
		return root->getTextGlyphInstanceBuffer().addInstanceData(d);
	}

	float TextBox::getAdvanceForChar(size_t i, const GlyphInfo& g, const EngineCore::FrameContext& f) const
	{
		const float kerning = (i == text.size() - 1) ? 0.f : font->getKerning(text[i], text[i + 1]);
		// convert (advance + kerning) to normalized [0, 1] width
		return (g.advance + kerning) * fontScale / f.viewport.extent.x;
	}


}