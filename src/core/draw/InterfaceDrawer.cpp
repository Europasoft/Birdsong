#include "core/draw/InterfaceDrawer.h"
#include "core/draw/FrameContext.h"
#include "core/ui/TextBox.h"
#include "core/ui/Box.h"
#include "core/gpu/Material.h"
#include "core/gpu/descriptors/InstanceBuffer.h"
#include "core/ui/Element.h"
#include "core/ui/Fonts.h"
#include "core/gpu/Device.h"
#include "core/engine/Camera.h"
#include "core/gpu/descriptors/BindlessTextureManager.h"
#include "core/world/Scene.h"

#include <stdexcept>
#include <array>
#include <limits>
#include <string>
#include <iostream> // temporary

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace EngineCore
{
	using namespace UI;
	using namespace WorldSystem;
	using namespace ShaderPushConstants;

	InterfaceDrawer::InterfaceDrawer(EngineDevice& device, const DrawContext& d)
		: DrawBase(device, d)
	{
		root = RootElement::create(device);
		
		// load font
		auto& texMgr = d.world->getScene().getTextureManager();
		fonts.push_back(Font::load(device, "fonts/InterDisplay-Regular.ttf", texMgr));

		HorizontalBox& box = root->addElement<HorizontalBox>();
		box.loadMaterial(device, d);
		box.size = Vec2(0.66f);
		box.position = Vec2(0.5f);
		box.pivotPoint = Vec2(0.5f);
		box.backgroundColor = Vec(0.1f, 0.1f, 0.6f);
		box.backgroundOpacity = 0.1f;

		TextBox& text = box.addElement<TextBox>();
		text.loadMaterial(device, d);
		text.setFont(fonts[0]);
		text.text = U"Hello";
		text.fontScale = 60;
		text.size = Vec2(0.66f, 0.25f);
		text.position = Vec2(0.0f);
		text.pivotPoint = Vec2(0.f, 0.f);
		text.backgroundColor = Vec(0.2f, 0.2f, 0.5f);
		text.cornerRadiusTop = Vec2(8.f, 32.f);
		text.cornerRadiusBottom = Vec2(32.f, 8.f);

		TextBox& text2 = box.addElement<TextBox>();
		text2.loadMaterial(device, d);
		text2.setFont(fonts[0]);
		text2.text = U"abc 123 \u00E5\u00E4\u00F6 \u00C5\u00C4\u00D6 \u00E0 \u00E1 Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod";
		text2.size = Vec2(0.33f, 0.2f);
		text2.position = Vec2(0.0f);
		text2.pivotPoint = Vec2(0.f, 0.f);
		text2.backgroundColor = Vec(0.15f, 0.15f, 0.25f);
		text2.backgroundOpacity = 0.3f;
		text2.alignHorizontal = UI::TextBox::EAlignH::CENTER;
		text2.alignVertical = UI::TextBox::EAlignV::CENTER;
		text2.setCornerRadius(64.f);
	}

	InterfaceDrawer::~InterfaceDrawer() = default;

	void InterfaceDrawer::render(const FrameContext& f)
	{

		root->drawAll(f);
	}

	bool InterfaceElement::cursorHitTest(glm::vec2 cursor) const
	{
		return (cursor.x <= position.x + size.x/2) && (cursor.x >= position.x) &&
				(cursor.y <= position.y + size.y/2) && (cursor.y >= position.y);
	}

	


}