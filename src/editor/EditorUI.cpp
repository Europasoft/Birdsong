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
		e->hoverBackgroundColor = Vec(0.16f);
		e->setCornerRadius(0.f);
	}

	EditorUI::EditorUI(EngineCore::EngineDevice& device)
	{
		topBarHeight = 0.02f;
		bottomBarHeight = 0.02f;
		rightStackWidth = 0.2f;
		leftStackWidth = 1.f - rightStackWidth;
		viewportHeight = 0.75f;
		contentBrowserHeight = 1.f - viewportHeight;

		rootElement = RootElement::create(device);

		// vertical stack that contains all editor elements
		rootStackElement = &rootElement->addElement<VerticalBox>();
		editorElementDefaults(rootStackElement);
		rootStackElement->backgroundOpacity = 0.f;

		createEditorTopBar();
		createEditorMainArea();
		createEditorBottomBar();
	}

	void EditorUI::createEditorMainArea()
	{
		// container for all the central editor elements
		editorMainArea = &rootStackElement->addElement<HorizontalBox>();
		editorElementDefaults(editorMainArea);
		editorMainArea->size.y = 1.f - (topBarHeight + bottomBarHeight);
		editorMainArea->backgroundOpacity = 0.f;

		createEditorLeftStack();
		createEditorRightStack();
	}

	void EditorUI::createEditorRightStack()
	{
		// vertical stack containing the right side menus
		editorRightStackElement = &editorMainArea->addElement<VerticalBox>();
		editorElementDefaults(editorRightStackElement);
		editorRightStackElement->size.x = rightStackWidth;
		editorRightStackElement->backgroundOpacity = 1.f;
		editorRightStackElement->cursorBehavior = Element::ECursorBehavior::RESPOND_PASS;
	}

	void EditorUI::createEditorLeftStack()
	{
		// vertical stack containing the viewport and asset browser
		editorLeftStackElement = &editorMainArea->addElement<VerticalBox>();
		editorElementDefaults(editorLeftStackElement);
		editorLeftStackElement->size.x = leftStackWidth;

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
	}

	void EditorUI::createEditorTopBar()
	{
		// bar at the very top of the window surface
		editorTopBar = &rootStackElement->addElement<HorizontalBox>();
		editorElementDefaults(editorTopBar);
		editorTopBar->size.y = topBarHeight;
		editorTopBar->backgroundOpacity = 1.f;

		// editor title text
		topBarTitleText = &editorTopBar->addElement<TextBox>();
		editorElementDefaults(topBarTitleText);
		topBarTitleText->text = U"Birdsong Editor";
		topBarTitleText->fontScale = 16.f;
		topBarTitleText->backgroundOpacity = 1.f;
		topBarTitleText->alignHorizontal = UI::TextBox::EAlignH::LEFT;
		topBarTitleText->alignVertical = UI::TextBox::EAlignV::CENTER;
	}

	void EditorUI::createEditorBottomBar()
	{
		// bar at the very bottom of the window surface
		editorBottomBar = &rootStackElement->addElement<HorizontalBox>();
		editorElementDefaults(editorBottomBar);
		editorBottomBar->size.y = bottomBarHeight;
		editorBottomBar->backgroundOpacity = 1.f;
		editorBottomBar->cursorBehavior = Element::ECursorBehavior::RESPOND_PASS;
	}

	void EditorUI::loadMaterials(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d)
	{
		DrawContext dd = d;
		dd.samples = VK_SAMPLE_COUNT_1_BIT; // editor UI is drawn directly to the swapchain, no MSAA

		rootElement->loadMaterialsRecursive(device, dd);
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

		topBarTitleText->setFont(editorDefaultFont);
	}
}