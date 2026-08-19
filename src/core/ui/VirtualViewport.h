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
		void init();

		virtual void loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d) override;

		const EngineCore::ViewportState& getViewportState() const;

	protected:
		virtual void preDraw(const EngineCore::FrameContext& f, const PreDrawData& data, Vec2 renderPosition) override;
		virtual void draw(const EngineCore::FrameContext& f, EngineCore::Material*& m) override;

		EngineCore::ViewportState viewportState{}; // extent and position based on the shape of this UI element

	};

}