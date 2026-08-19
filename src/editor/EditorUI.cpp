// Copyright 2026 Simon Liimatainen. All rights reserved.
#include "editor/EditorUI.h"
#include "core/draw/FrameContext.h"
#include "core/ui/TextBox.h"
#include "core/ui/Box.h"
#include "core/ui/Element.h"
#include "core/ui/Fonts.h"
#include "core/ui/VirtualViewport.h"

namespace Editor
{
	using namespace EngineCore;
	using namespace UI;

	static std::string_view editorDefaultFontPath = "fonts/InterDisplay-Regular.ttf";

	EditorUI::~EditorUI() = default;

	EditorUI::EditorUI(EngineCore::EngineDevice& device)
	{
		rootElement = RootElement::create(device);

		editorStackElement = &rootElement->addElement<VerticalBox>();
		editorStackElement->position = Vec2(0.01f);
		editorStackElement->size = Vec2(0.98f, 0.1f);
		editorStackElement->pivotPoint = Vec2(0.f);
		editorStackElement->backgroundColor = Vec(0.1f, 0.1f, 0.6f);

		viewportElement = &editorStackElement->addElement<VirtualViewport>();

		/*
		HorizontalBox& box = rootElement->addElement<HorizontalBox>();
		box.loadMaterial(device, d);
		box.size = Vec2(0.66f);
		box.position = Vec2(0.5f);
		box.pivotPoint = Vec2(0.5f);
		box.backgroundColor = Vec(0.1f, 0.1f, 0.6f);
		box.backgroundOpacity = 0.1f;
		box.hoverBackgroundColor = box.backgroundColor;
		box.hoverBackgroundOpacity = box.backgroundOpacity + 0.05;

		TextBox& text = box.addElement<TextBox>();
		text.loadMaterial(device, d);
		text.setFont(editorDefaultFont);
		text.text = U"Hello";
		text.fontScale = 60;
		text.size = Vec2(0.66f, 0.25f);
		text.position = Vec2(0.0f);
		text.pivotPoint = Vec2(0.f, 0.f);
		text.backgroundColor = Vec(0.2f, 0.2f, 0.5f);
		text.hoverBackgroundColor = text.backgroundColor + 0.1f;
		text.cornerRadiusTop = Vec2(8.f, 32.f);
		text.cornerRadiusBottom = Vec2(32.f, 8.f);

		TextBox& text2 = box.addElement<TextBox>();
		text2.loadMaterial(device, d);
		text2.setFont(editorDefaultFont);
		text2.text = U"Editor Abc 123 \u00E5\u00E4\u00F6 \u00C5\u00C4\u00D6 \u00E0 \u00E1 Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod";
		text2.size = Vec2(0.33f, 0.2f);
		text2.position = Vec2(0.0f);
		text2.pivotPoint = Vec2(0.f, 0.f);
		text2.backgroundColor = Vec(0.15f, 0.15f, 0.25f);
		text2.backgroundOpacity = 0.3f;
		text2.hoverBackgroundColor = text2.backgroundColor + 0.1f;
		text2.alignHorizontal = UI::TextBox::EAlignH::CENTER;
		text2.alignVertical = UI::TextBox::EAlignV::CENTER;
		text2.setCornerRadius(35.f);
		*/
	}

	void EditorUI::loadMaterials(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d)
	{
		DrawContext dd = d;
		dd.samples = VK_SAMPLE_COUNT_1_BIT; // editor UI is drawn directly to the swapchain, no MSAA

		editorStackElement->loadMaterial(device, dd);
	}

	void EditorUI::render(EngineDevice& device, const FrameContext& f, const DrawContext& d)
	{
		if (not editorDefaultFont) loadDefaultFonts(device, d);

		rootElement->drawAll(f);
	}

	const EngineCore::ViewportState& EditorUI::getViewportState() const
	{
		assert(viewportElement);
		return viewportElement->getViewportState();
	}

	void EditorUI::loadDefaultFonts(EngineDevice& device, const DrawContext& d)
	{
		auto& texMgr = d.world->getScene().getTextureManager();
		editorDefaultFont = Font::load(device, editorDefaultFontPath, texMgr);
	}

	

}