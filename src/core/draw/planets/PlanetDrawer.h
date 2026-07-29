#pragma once
#include "core/gpu/Material.h"
#include "core/include/shared/Transform.h"

#include "core/types/vk.h"

#include <memory>
#include <vector>

namespace WorldSystem
{ 
	class EngineNodeData;
	class World;
}

namespace EngineCore
{
	class EngineDevice;
	class Material;

	struct PlanetNodeContext
	{
		std::vector<std::unique_ptr<WorldSystem::EngineNodeData>> faces;
		std::shared_ptr<Material> material;
	};

	class PlanetDrawer
	{
	public:
		PlanetDrawer(EngineDevice& device, WorldSystem::World& world, const RenderingFormats& formats, VkSampleCountFlagBits samples);
		~PlanetDrawer();

		PlanetDrawer(const PlanetDrawer&) = delete;
		PlanetDrawer& operator=(const PlanetDrawer&) = delete;

		void render(VkCommandBuffer commandBuffer, uint32_t frameIndex);

	private:
		EngineDevice& device;
		WorldSystem::World& world;
		std::vector<std::unique_ptr<PlanetNodeContext>> planets;
	};

}