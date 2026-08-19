// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once

#include <memory>
#include <string_view>

namespace EngineCore
{
	class EngineDevice;
	struct DrawContext;
	struct FrameContext;
}

namespace UI
{
	class RootElement;
	class Font;
	struct GlyphInfo;
	class TextBox;
}

namespace Editor
{
	class EditorUI
	{
	public:
		EditorUI(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d);
		~EditorUI();

		void render(EngineCore::EngineDevice& device, const EngineCore::FrameContext& f, const EngineCore::DrawContext& d);

	protected:
		std::unique_ptr<UI::RootElement> rootElement;
		std::shared_ptr<UI::Font> editorDefaultFont;

	protected:
		void buildUI(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d);
		void loadDefaultFonts(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d);

	};

}