#pragma once
#include "core/draw/DrawBase.h"
#include "core/types/vk.h"
#include "core/types/CommonTypes.h"
#include <array>
#include <memory>
#include <vector>

namespace WorldSystem
{
	class EngineNodeData;
}

namespace EngineCore
{
	class EngineDevice;
	class Renderer;
	class DescriptorSet;
	class Material;
	struct RenderingFormats;

	namespace ShaderPushConstants { struct DebugPrimitivePushConstants; }

	class DebugDrawer : public DrawBase
	{
	public:
		DebugDrawer(EngineDevice& device, const DrawContext& d);
		~DebugDrawer();

		void addDebugBox(Vec dimensions, Vec location, Vec color, float opacity = 1.f);
		void removeDebugBoxes();

		virtual void render(const FrameContext& f) override;

	private:
		using DDPushConstant = ShaderPushConstants::DebugPrimitivePushConstants;
		std::unique_ptr<WorldSystem::EngineNodeData> enodeBox;
		std::vector<DDPushConstant> boxPushConstants;

		bool hasPushConstantBox(const DDPushConstant& compareBox) const;
	};

}
