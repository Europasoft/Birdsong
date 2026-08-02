// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once
#include "core/gpu/Material.h"
#include "core/include/shared/Transform.h"
#include "core/types/CommonTypes.h"

#include "core/types/vk.h"

#include <memory>
#include <vector>
#include <atomic>
#include <thread>

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
	struct JunkPileItem;

	static constexpr float finalDrawDistance = 800;

	struct Planet
	{
		std::vector<std::unique_ptr<TerrainPatch>> roots;
		std::shared_ptr<Material> material;
		Transform transform;
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

		void render(VkCommandBuffer commandBuffer, uint32_t frameIndex, const Transform& cameraTransform, double dt);

	private:
		EngineDevice& device;
		WorldSystem::World& world;
		std::vector<std::unique_ptr<Planet>> planets;
		std::atomic<bool> asyncInProgress = false;
		std::thread asyncThread;

		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
		Transform camTransform;
		uint32_t currentFrameIndex;
		double delta = 0.0;
		double currentXoffset = 800.f;
		double tempTimer = 0.6;

		std::vector<std::unique_ptr<JunkPileItem>> junkPile; // old patches to be deleted when the GPU is done with them

		void regenerate(Planet& planet);

		void updateLOD();
		void evaluatePatchLOD(TerrainPatch& patch, Planet& planet);

		void drawRecursive(TerrainPatch& patch, Planet& planet);
		void drawLeafPatch(TerrainPatch& patch, Planet& planet);

		void cleanJunkPile();

	};

	

}