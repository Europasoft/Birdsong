#pragma once
#include "core/ui/Element.h"

#include <string>

namespace EngineCore
{
	struct FrameContext;
	struct DrawContext;
	class Material;
}

namespace UI
{
	class Font;
	struct GlyphInfo;
	struct ScaledFontMetrics;

	struct Line
	{
		size_t startIndex;
		size_t length;
		float width;
	};

	class TextBox : public Element
	{
	public:
		TextBox();
		virtual ~TextBox();

		std::u32string text;
		float fontScale = 24.f;
		enum class EAlignV : uint32_t { CENTER, TOP, BOTTOM };
		EAlignV alignVertical = EAlignV::CENTER;
		enum class EAlignH : uint32_t { LEFT, CENTER, RIGHT };
		EAlignH alignHorizontal = EAlignH::CENTER;

		void setFont(std::shared_ptr<Font> newFont);
		virtual void loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d) override;

	protected:
		virtual void preDraw(const EngineCore::FrameContext& f, const PreDrawData& data, Vec2 renderPosition) override;
		virtual void draw(const EngineCore::FrameContext& f, EngineCore::Material*& m) override;

		std::vector<Line> processLines(const EngineCore::FrameContext& f, const PreDrawData& data) const;
		void generateInstances(const std::vector<Line>& lines, const ScaledFontMetrics& metrics,
							const EngineCore::FrameContext& f, const PreDrawData& data);
		uint32_t addGlyphToInstanceBuffer(const UI::GlyphInfo& g, float offset, const Vec2& basePosition);
		float getAdvanceForChar(size_t i, const GlyphInfo& g, const EngineCore::FrameContext& f) const;

	protected:
		std::shared_ptr<EngineCore::Material> textMaterial;
		std::shared_ptr<Font> font;
		uint32_t firstGlyphInstanceBufferID = 0;
	};

}