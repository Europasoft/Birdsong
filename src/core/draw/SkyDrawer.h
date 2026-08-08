#pragma once
#include "core/draw/DrawBase.h"
#include "core/types/vk.h"
#include "core/types/CommonTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace WorldSystem
{
	class EngineNodeData;
	class World;
}

namespace EngineCore
{
	class EngineDevice;
	class DescriptorSet;
	struct RenderingFormats;

	class SkyDrawer : public DrawBase
	{
	public:
		SkyDrawer(EngineDevice& device, const DrawContext& d);
		~SkyDrawer();

		virtual void render(const FrameContext& f) override;

		float skyMeshScale = 1000.f * 10.f;

	private:
		std::unique_ptr<WorldSystem::EngineNodeData> enodeSky;
		
	};

}
