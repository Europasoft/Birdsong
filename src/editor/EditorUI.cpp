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

	void editorElementDefaults(Element* e)
	{
		e->position = Vec2(0.f);
		e->pivotPoint = Vec2(0.f);
		e->size = Vec2(1.f);
		e->cursorBehavior = Element::ECursorBehavior::IGNORE;
		e->backgroundColor = Vec(0.1f);
		e->backgroundOpacity = 0.f;
		e->hoverBackgroundColor = Vec(0.16);
	}

	EditorUI::EditorUI(EngineCore::EngineDevice& device)
	{
		rootElement = RootElement::create(device);

		const float topBarHeight = 0.02f;
		const float bottomBarHeight = 0.02f;
		const float rightStackWidth = 0.2f;
		const float leftStackWidth = 1.f - rightStackWidth;
		const float viewportHeight = 0.75f;
		const float contentBrowserHeight = 1.f - viewportHeight;

		// vertical stack that contains all editor elements
		rootStackElement = &rootElement->addElement<VerticalBox>();
		editorElementDefaults(rootStackElement);
		rootStackElement->backgroundOpacity = 0.f;

		// bar at the very top of the window surface
		editorTopBar = &rootStackElement->addElement<HorizontalBox>();
		editorElementDefaults(editorTopBar);
		editorTopBar->size.y = topBarHeight;
		editorTopBar->backgroundOpacity = 1.f;
		editorTopBar->cursorBehavior = Element::ECursorBehavior::RESPOND_PASS;

		// container for all the central editor elements
		editorMainArea = &rootStackElement->addElement<HorizontalBox>();
		editorElementDefaults(editorMainArea);
		editorMainArea->size.y = 1.f - (topBarHeight + bottomBarHeight);
		editorMainArea->backgroundOpacity = 0.f;


		// vertical stack containing the viewport and asset browser
		editorLeftStackElement = &editorMainArea->addElement<VerticalBox>();
		editorElementDefaults(editorLeftStackElement);
		editorLeftStackElement->size.x = leftStackWidth;

		// vertical stack containing the right side menus
		editorRightStackElement = &editorMainArea->addElement<VerticalBox>();
		editorElementDefaults(editorRightStackElement);
		editorRightStackElement->size.x = rightStackWidth;
		editorRightStackElement->backgroundOpacity = 1.f;
		editorRightStackElement->cursorBehavior = Element::ECursorBehavior::RESPOND_PASS;

		// virtual viewport area cutout
		viewportElement = &editorLeftStackElement->addElement<VirtualViewport>();
		editorElementDefaults(viewportElement);
		viewportElement->size.y = viewportHeight;

		// bottom content browser
		contentBrowserElement = &editorLeftStackElement->addElement<HorizontalBox>();
		editorElementDefaults(contentBrowserElement);
		contentBrowserElement->size.y = contentBrowserHeight;
		contentBrowserElement->backgroundOpacity = 1.f;
		contentBrowserElement->cursorBehavior = Element::ECursorBehavior::RESPOND_PASS;


		// bar at the very bottom of the window surface
		editorBottomBar = &rootStackElement->addElement<HorizontalBox>();
		editorElementDefaults(editorBottomBar);
		editorBottomBar->size.y = bottomBarHeight;
		editorBottomBar->backgroundOpacity = 1.f;
		editorBottomBar->cursorBehavior = Element::ECursorBehavior::RESPOND_PASS;

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

		rootStackElement->loadMaterial(device, dd);
		editorTopBar->loadMaterial(device, dd);
		editorBottomBar->loadMaterial(device, dd);
		editorMainArea->loadMaterial(device, dd);
		editorLeftStackElement->loadMaterial(device, dd);
		editorRightStackElement->loadMaterial(device, dd);
		contentBrowserElement->loadMaterial(device, dd);
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