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
	class HorizontalBox;
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

		UI::VerticalBox* rootStackElement = nullptr;
		UI::HorizontalBox* editorTopBar = nullptr;
		UI::HorizontalBox* editorBottomBar = nullptr;
		UI::HorizontalBox* editorMainArea = nullptr;
		UI::VerticalBox* editorLeftStackElement = nullptr;
		UI::VerticalBox* editorRightStackElement = nullptr;
		UI::VirtualViewport* viewportElement = nullptr;
		UI::HorizontalBox* contentBrowserElement = nullptr;
		UI::TextBox* topBarTitleText = nullptr;

		void createEditorMainArea();
		void createEditorRightStack();
		void createEditorLeftStack();
		void createEditorTopBar();
		void createEditorBottomBar();

		float topBarHeight;
		float bottomBarHeight;
		float rightStackWidth;
		float leftStackWidth;
		float viewportHeight;
		float contentBrowserHeight;

	protected:
		void loadDefaultFonts(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d);

	};

}