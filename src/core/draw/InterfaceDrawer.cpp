#include "core/draw/InterfaceDrawer.h"
#include "core/draw/FrameContext.h"
#include "core/ui/TextBox.h"
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
		fonts.push_back(Font::load(device, "fonts/Inter-VariableFont_opsz,wght.ttf", texMgr));

		VerticalBox& box = root->addElement<VerticalBox>();
		box.loadMaterial(device, d);
		box.size = Vec2(0.33f);
		box.position = Vec2(0.5f);
		box.pivotPoint = Vec2(0.5f);
		box.backgroundColor = Vec(0.1f, 0.1f, 0.6f);
		box.backgroundOpacity = 0.5f;

		TextBox& text = box.addElement<TextBox>();
		text.loadMaterial(device, d);
		text.setFont(fonts[0]);
		text.text = "Hello!";
		text.size = Vec2(1.f, 0.33f);
		text.position = Vec2(0.0f);
		text.pivotPoint = Vec2(0.f, 0.f);
		text.backgroundColor = Vec(0.2f, 0.2f, 0.5f);
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