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
		position = Vec2(0.f);
		pivotPoint = Vec2(0.f);

		// set the extent and size once at start (testing)
		viewportState = EngineCore::ViewportState
		{
			.extent = Vec2(50, 50),
			.position = Vec2(0.f, 0.f)
		};
	}

	VirtualViewport::~VirtualViewport() = default;

	const EngineCore::ViewportState& VirtualViewport::getViewportState() const
	{
		return viewportState; // in pixels
	}

	void VirtualViewport::preDrawRecursive(const EngineCore::FrameContext& f, const PreDrawData& parentData)
	{
		PreDrawData currentData = {};
		const Vec2 parentTopLeft = parentData.position - parentData.pivot;
		currentData.size = parentData.size * size; // calculate absolute size
		currentData.position = parentTopLeft + (position * parentData.size); // calculate top-left origin ignoring parent pivot
		currentData.pivot = pivotPoint * currentData.size; // store absolute pivot offset for this element
		const Vec2 renderPosition = currentData.position - currentData.pivot;

		// calculate absolute position and scale in pixels, for viewport content (ViewportDrawer)
		viewportState.extent = currentData.size * f.viewport.extent;
		viewportState.position = renderPosition * f.viewport.extent;

		handleInput(f, currentData, renderPosition);

		// viewport may have child elements (must be drawn after viewport content to be visible)
		preDrawNested(f, currentData);
	}

	void VirtualViewport::handleInput(const EngineCore::FrameContext& f, PreDrawData& currentData, Vec2 renderPosition)
	{
		currentData.hovered = cursorHitTest(f, renderPosition, currentData.size);
		currentData.clicked = (currentData.hovered && f.leftClick);
	}

}