#pragma once
#include "Core/Types/vk.h"
#include "Core/Types/CommonTypes.h"
#include <array>
#include <memory>
#include <vector>

namespace EngineCore
{
	class EngineDevice;
	class Renderer;
	class DescriptorSet;
	class Primitive;
	class Material;
	struct RenderingFormats;

	namespace ShaderPushConstants { struct DebugPrimitivePushConstants; }

	class DebugDrawer
	{
	public:
		DebugDrawer(EngineDevice& device, DescriptorSet& defaultSet, const RenderingFormats& formats, VkSampleCountFlagBits samples);

		void addDebugBox(Vec dimensions, Vec location, Vec color, float opacity = 1.f);
		void removeDebugBoxes();

		void render(VkCommandBuffer cmdBuffer, Renderer& renderer);

	private:
		using DDPushConstant = ShaderPushConstants::DebugPrimitivePushConstants;

		EngineDevice& device;
		DescriptorSet& defaultSet;
		std::unique_ptr<Primitive> boxMesh;
		std::vector<DDPushConstant> boxPushConstants;

		bool hasPushConstantBox(const DDPushConstant& compareBox) const;
	};

}
