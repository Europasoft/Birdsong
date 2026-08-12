#include "core/ui/Box.h"
#include "core/draw/FrameContext.h"

namespace UI
{
	VerticalBox::VerticalBox() = default;
	VerticalBox::~VerticalBox() = default;

	float scaleToFit(float& accumulatedRelative, float space, float elementSize, const PreDrawData& currentData)
	{
		// results in an effective container size that scales the nested element's size to allocatedRelative
		const float allocatedRelative = std::min(elementSize, std::max(0.0f, 1.0f - accumulatedRelative));
		const float scaleFactor = allocatedRelative / elementSize;
		accumulatedRelative += allocatedRelative;
		return space * scaleFactor; // just returns space if element fits without scaling
	}

	void VerticalBox::preDrawNested(const EngineCore::FrameContext& f, const PreDrawData& currentData)
	{
		const float containerSpace = currentData.size.y;
		float accumulatedRelative = 0.0f;
		for (auto& e : nested)
		{
			PreDrawData hData = currentData;
			hData.position.y = currentData.position.y + (accumulatedRelative * containerSpace);

			const float containerReportedSize = scaleToFit(accumulatedRelative, containerSpace, e->size.y, currentData);
			hData.size.y = (e->size.y > 0.0f) ? containerReportedSize : 0.f; // container reports smaller size if element would overflow

			e->preDrawRecursive(f, hData);
		}
	}

	HorizontalBox::HorizontalBox() = default;
	HorizontalBox::~HorizontalBox() = default;

	void HorizontalBox::preDrawNested(const EngineCore::FrameContext& f, const PreDrawData& currentData)
	{
		const float containerSpace = currentData.size.x;
		float accumulatedRelative = 0.0f;
		for (auto& e : nested)
		{
			PreDrawData hData = currentData;
			hData.position.x = currentData.position.x + (accumulatedRelative * containerSpace);

			const float containerReportedSize = scaleToFit(accumulatedRelative, containerSpace, e->size.x, currentData);
			hData.size.x = (e->size.x > 0.0f) ? containerReportedSize : 0.f;

			e->preDrawRecursive(f, hData);
		}
	}

}