#pragma once
#include "core/ui/Element.h"
#include "core/types/CommonTypes.h"
#include "core/draw/FrameContext.h"

#include <string>

namespace EngineCore
{
	struct FrameContext;
	struct DrawContext;
	class Material;
}

namespace UI
{
	// this element controls where (and how big) the 3D scene goes when rendering in editor
	// the actual viewport content is filled by the ViewportDrawer
	class VirtualViewport : public Element
	{
	public:
		VirtualViewport();
		virtual ~VirtualViewport();

		const EngineCore::ViewportState& getViewportState() const;

		void preDrawRecursive(const EngineCore::FrameContext& f, const PreDrawData& parentData) override;

		// the viewport element doesn't draw anything on its own, it only makes space for the content
		void loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d) override {}; // no-op

	protected:
		void draw(const EngineCore::FrameContext& f, EngineCore::Material*& m) override {}; // no-op
		
		void handleInput(const EngineCore::FrameContext& f, PreDrawData& currentData, Vec2 renderPosition) override;

		EngineCore::ViewportState viewportState{}; // extent and position based on the shape of this UI element

	};

}