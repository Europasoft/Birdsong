#include "core/ui/VirtualViewport.h"
#include "core/ui/Fonts.h"
#include "core/gpu/Material.h"
#include "core/draw/FrameContext.h"
#include "core/gpu/descriptors/InstanceBuffer.h"
#include "core/render/Renderer.h"
#include "core/draw/ViewportDrawer.h"

#include "core/types/vk.h"

#include <algorithm>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <cctype>

namespace UI
{
	VirtualViewport::VirtualViewport()
	{
		// set the extent and size once at start (testing)
		viewportState = EngineCore::ViewportState
		{
			.extent = Vec2(900, 900),
			.position = Vec2(0.f, 0.f)
		};
	}

	VirtualViewport::~VirtualViewport() = default;

	void VirtualViewport::loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d)
	{
	}

	const EngineCore::ViewportState& VirtualViewport::getViewportState() const
	{
		return viewportState; // in pixels
	}

	void VirtualViewport::preDraw(const EngineCore::FrameContext& f, const PreDrawData& data, Vec2 renderPosition)
	{
	}

	void VirtualViewport::draw(const EngineCore::FrameContext& f, EngineCore::Material*& m)
	{
		// TODO: change viewportState when element resized
	}

}