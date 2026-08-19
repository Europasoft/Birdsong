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
	VirtualViewport::VirtualViewport() = default;

	VirtualViewport::~VirtualViewport() = default;

	void VirtualViewport::init(const EngineCore::DrawContext& d)
	{
		renderer = d.renderer;
		drawer = d.viewportDrawer;
		reportViewportShapeChanged(); // set the extent and size once before rendering first frame
	}

	void VirtualViewport::loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d)
	{
	}

	void VirtualViewport::preDraw(const EngineCore::FrameContext& f, const PreDrawData& data, Vec2 renderPosition)
	{
	}

	void VirtualViewport::draw(const EngineCore::FrameContext& f, EngineCore::Material*& m)
	{
		reportViewportShapeChanged(); // TODO: only call when element resized
	}

	void VirtualViewport::reportViewportShapeChanged()
	{
		assert(renderer && drawer);
		if (renderer && drawer)
		{
			// TODO: temporarily hardcoded. also figure out how to create/recreate the attachment images at the right resolution!
			renderer->setViewportExtent({ 1400, 900 });
			drawer->setPositionAndSize({ 0, 0 }, { 1400, 900 });
		}
	}

}