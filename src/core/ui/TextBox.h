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

	class TextBox : public HorizontalBox
	{
	public:
		TextBox();
		virtual ~TextBox();

		std::string text;
		float fontScale = 24.f;

		void setFont(std::shared_ptr<Font> newFont);
		virtual void loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d) override;

	protected:
		virtual void preDraw(const EngineCore::FrameContext& f, const PreDrawData& data) override;
		virtual void draw(const EngineCore::FrameContext& f, EngineCore::Material*& m) override;

		uint32_t addGlyphToInstanceBuffer(const UI::GlyphInfo& g, float offset, const Vec2& basePosition);

	protected:
		std::shared_ptr<EngineCore::Material> textMaterial;
		std::shared_ptr<Font> font;
		uint32_t firstGlyphInstanceBufferID = 0;
	};

}