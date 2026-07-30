#pragma once
#include "core/gpu/Material.h"
#include "core/include/shared/Transform.h"
#include "core/types/CommonTypes.h"

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
	class TerrainPatch;

	struct Planet
	{
		std::vector<std::unique_ptr<TerrainPatch>> roots;
		std::shared_ptr<Material> material;
		Transform centerTransform{};
		uint32_t resolution = 0;
		double radius = 0;
	};

	class PlanetDrawer
	{
	public:
		PlanetDrawer(EngineDevice& device, WorldSystem::World& world, const RenderingFormats& formats, VkSampleCountFlagBits samples);
		~PlanetDrawer();

		PlanetDrawer(const PlanetDrawer&) = delete;
		PlanetDrawer& operator=(const PlanetDrawer&) = delete;

		void regenerate(Planet& planet);

		void render(VkCommandBuffer commandBuffer, uint32_t frameIndex, const Transform& cameraTransform, double dt);

	private:
		EngineDevice& device;
		WorldSystem::World& world;
		std::vector<std::unique_ptr<Planet>> planets;

		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
		Transform camTransform;
		double delta = 0.0;
		double currentXoffset = 800.f;
		double tempTimer = 0.6;

		//void updateLOD(Quad& quad, const Vec64& cameraPos, std::shared_ptr<Material> material, ResRad& r);
		//void mergeQuad(Quad& quad);

		void drawRecursive(TerrainPatch& patch, Planet& planet);
		void drawLeafPatch(TerrainPatch& patch, Planet& planet);
	};

}