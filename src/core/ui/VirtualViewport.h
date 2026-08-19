#pragma once
#include "core/ui/Element.h"

#include <string>

namespace EngineCore
{
	struct FrameContext;
	struct DrawContext;
	class Material;
	class Renderer;
	class ViewportDrawer;
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
		void init(const EngineCore::DrawContext& d);

		virtual void loadMaterial(EngineCore::EngineDevice& device, const EngineCore::DrawContext& d) override;

	protected:
		virtual void preDraw(const EngineCore::FrameContext& f, const PreDrawData& data, Vec2 renderPosition) override;
		virtual void draw(const EngineCore::FrameContext& f, EngineCore::Material*& m) override;
		void reportViewportShapeChanged();

		EngineCore::Renderer* renderer = nullptr;
		EngineCore::ViewportDrawer* drawer = nullptr;

	};

}