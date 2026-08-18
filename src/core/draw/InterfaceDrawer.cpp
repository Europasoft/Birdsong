// Copyright 2026 Simon Liimatainen. All rights reserved.
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
#include "core/asset/Collection.h"
#include "core/asset/glTFLoader.h"

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
	}

	InterfaceDrawer::~InterfaceDrawer() = default;

	void InterfaceDrawer::render(const FrameContext& f)
	{
	}

}