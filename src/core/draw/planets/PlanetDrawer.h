// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once
#include "core/gpu/Material.h"
#include "core/include/shared/Transform.h"
#include "core/types/CommonTypes.h"

#include "core/types/vk.h"

#include <memory>
#include <vector>
#include <array>
#include <thread>
#include <future>
#include <mutex>

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
	class AsyncCommandBuffer;

	static constexpr float finalDrawDistance = 80000;

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

		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
		Transform camTransform;
		uint32_t currentFrameIndex = 0;
		double delta = 0.0;

		std::thread updater;
		std::atomic<bool> updaterIdle = true;
		std::atomic<bool> updaterExit = false;
		std::mutex updaterDoneMutex;
		std::unique_ptr<AsyncCommandBuffer> asyncCmdBuffer;
		std::atomic<bool> firstGeometryReady = false;

		std::array<std::vector<std::unique_ptr<JunkPileItem>>, 2> junkPiles; // old patches to be deleted when the GPU is done with them
		std::array<std::mutex, 2> junkPileMutexes; // used to guarantee that we don't write and free on the same junk pile
		std::vector<std::unique_ptr<JunkPileItem>>& getJunkPile(uint32_t threadIndex);
		void ensureJunkPileLock(uint32_t threadIndex, std::unique_lock<std::mutex>& lock);

		void createRootFaces();
		void asyncUpdate();
		void evaluatePatchLOD(TerrainPatch& patch, Planet& planet);
		uint32_t checkSplitCriteria(TerrainPatch& patch, Planet& planet);

		void attemptReplace(std::unique_ptr<TerrainPatch>& patch);
		void drawRecursive(std::unique_ptr<TerrainPatch>& patch, Planet& planet);
		void drawLeafPatch(std::unique_ptr<TerrainPatch>& patch, Planet& planet);

		int32_t acquireJunkPile(std::unique_lock<std::mutex>& lock, bool allowFail = false);
		void cleanJunkPile(uint32_t cleaningFrameIndex);

		double shaderReloadTimer = 2.5;
		bool reloadShadersNextFrame = false;
		void tickShaderDesignMode();
	};

}