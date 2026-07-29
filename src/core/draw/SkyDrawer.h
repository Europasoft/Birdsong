#pragma once
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

	class SkyDrawer 
	{
	public:
		SkyDrawer(EngineDevice& device, WorldSystem::World& world, const RenderingFormats& formats, VkSampleCountFlagBits samples);
		~SkyDrawer();

		void renderSky(uint32_t frameIndex, VkCommandBuffer commandBuffer, VkDescriptorSet sceneGlobalDescriptorSet, Vec observerPosition);

		float skyMeshScale = 1000.f * 10.f;

	private:
		std::unique_ptr<WorldSystem::EngineNodeData> enodeSky;
		WorldSystem::World& world;
		
	};

}
