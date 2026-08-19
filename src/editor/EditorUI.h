// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once

#include <memory>
#include <string_view>

namespace EngineCore
{
	class EngineDevice;
	struct DrawContext;
	struct FrameContext;
	struct ViewportState;
}

namespace UI
{
	class RootElement;
	class VerticalBox;
	class VirtualViewport;
	class Font;
	struct GlyphInfo;
	class TextBox;
}

namespace Editor
{
	class EditorUI
	{
	public:
		EditorUI(EngineCore::EngineDevice& device);
		~EditorUI();

		void loadMaterials(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d);

		void render(EngineCore::EngineDevice& device, const EngineCore::FrameContext& f, const EngineCore::DrawContext& d);

		const EngineCore::ViewportState& getViewportState() const;

	protected:
		std::unique_ptr<UI::RootElement> rootElement;
		std::shared_ptr<UI::Font> editorDefaultFont;
		UI::VerticalBox* editorStackElement = nullptr;
		UI::VirtualViewport* viewportElement = nullptr;

	protected:
		void loadDefaultFonts(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d);

	};

}