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

	class TextBox : public Element
	{
	public:
		TextBox();
		virtual ~TextBox();

		std::string text;
		float fontScale = 24.f;
		enum class EAlignV : uint32_t { CENTER, TOP, BOTTOM };
		EAlignV alignVertical = EAlignV::CENTER;
		enum class EAlignH : uint32_t { LEFT, CENTER, RIGHT };
		EAlignH alignHorizontal = EAlignH::CENTER;

		void setFont(std::shared_ptr<Font> newFont);
		virtual void loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d) override;

	protected:
		virtual void preDraw(const EngineCore::FrameContext& f, const PreDrawData& data) override;
		virtual void draw(const EngineCore::FrameContext& f, EngineCore::Material*& m) override;

		uint32_t addGlyphToInstanceBuffer(const UI::GlyphInfo& g, float offset, const Vec2& basePosition);
		Vec2 alignText(Vec2 textPosition, const EngineCore::FrameContext& f, const PreDrawData& data);
		float getAdvanceForChar(size_t i, const GlyphInfo& g, const EngineCore::FrameContext& f) const;

	protected:
		std::shared_ptr<EngineCore::Material> textMaterial;
		std::shared_ptr<Font> font;
		uint32_t firstGlyphInstanceBufferID = 0;
	};

}