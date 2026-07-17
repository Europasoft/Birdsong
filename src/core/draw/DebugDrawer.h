#pragma once
#include "core/types/vk.h"
#include "core/types/CommonTypes.h"
#include <array>
#include <memory>
#include <vector>

namespace Nodes
{
	class MeshNode;
}

namespace EngineCore
{
	class EngineDevice;
	class Renderer;
	class DescriptorSet;
	class Material;
	struct RenderingFormats;

	namespace ShaderPushConstants { struct DebugPrimitivePushConstants; }

	class DebugDrawer
	{
	public:
		DebugDrawer(EngineDevice& device, DescriptorSet& defaultSet, const RenderingFormats& formats, VkSampleCountFlagBits samples);
		~DebugDrawer();

		void addDebugBox(Vec dimensions, Vec location, Vec color, float opacity = 1.f);
		void removeDebugBoxes();

		void render(VkCommandBuffer cmdBuffer, Renderer& renderer);

	private:
		using DDPushConstant = ShaderPushConstants::DebugPrimitivePushConstants;

		EngineDevice& device;
		DescriptorSet& defaultSet;
		std::unique_ptr<Nodes::MeshNode> boxMesh;
		std::vector<DDPushConstant> boxPushConstants;

		bool hasPushConstantBox(const DDPushConstant& compareBox) const;
	};

}
