#include "core/ui/Box.h"
#include "core/draw/FrameContext.h"

namespace UI
{
	VerticalBox::VerticalBox() = default;
	VerticalBox::~VerticalBox() = default;

	void VerticalBox::preDrawNested(const EngineCore::FrameContext& f, const PreDrawData& currentData)
	{
		float offset = 0.0f;
		for (auto& e : nested)
		{
			PreDrawData vData = currentData;
			vData.position.y = currentData.position.y + offset;
			e->preDrawRecursive(f, vData);
			offset += e->size.y * currentData.size.y;
		}
	}

	HorizontalBox::HorizontalBox() = default;
	HorizontalBox::~HorizontalBox() = default;

	void HorizontalBox::preDrawNested(const EngineCore::FrameContext& f, const PreDrawData& currentData)
	{
		float offset = 0.0f;
		for (auto& e : nested)
		{
			PreDrawData hData = currentData;
			hData.position.x = currentData.position.x + offset;
			e->preDrawRecursive(f, hData);
			offset += e->size.x * currentData.size.x;
		}
	}

}