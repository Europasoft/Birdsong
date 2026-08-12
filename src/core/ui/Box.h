#pragma once
#include "core/ui/Element.h"

#include <string>

namespace EngineCore
{
	struct FrameContext;
	struct DrawContext;
	class Material;
}

namespace UI
{
	class VerticalBox : public Element
	{
	public:
		VerticalBox();
		virtual ~VerticalBox();

	protected:
		virtual void preDrawNested(const EngineCore::FrameContext& f, const PreDrawData& currentData) override;
	};

	class HorizontalBox : public Element
	{
	public:
		HorizontalBox();
		virtual ~HorizontalBox();

	protected:
		virtual void preDrawNested(const EngineCore::FrameContext& f, const PreDrawData& currentData) override;
	};

}