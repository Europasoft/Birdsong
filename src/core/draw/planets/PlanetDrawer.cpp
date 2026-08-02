// Copyright 2026 Simon Liimatainen. All rights reserved.
#include "core/draw/planets/PlanetDrawer.h"
#include "core/draw/planets/TerrainPatch.h"
#include "core/gpu/Device.h"
#include "core/gpu/Material.h"
#include "core/include/shared/Transform.h"
#include "core/types/glm_conversions.h"
#include "core/types/Math.h"

#include "core/engine/Camera.h"
#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/world/Sector.h"

#include <stdexcept>
#include <array>
#include <limits>
#include <utility>
#include <numbers>

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <chrono>
#include <iostream>
#include "core/engine/EngineClock.h"

namespace EngineCore
{
	using namespace WorldSystem;

	struct FaceBasis
	{
		Vec right;
		Vec up;
		Vec forward;
	};

	FaceBasis getFaceBasis(ETerrainPatchFaceDirection face)
	{
		switch (face)
		{
		case ETerrainPatchFaceDirection::A: return { {0,0,1}, {0,1,0}, {1,0,0} };
		case ETerrainPatchFaceDirection::B: return { {0,0,-1}, {0,1,0}, {-1,0,0} };
		case ETerrainPatchFaceDirection::C: return { {1,0,0}, {0,0,1}, {0,1,0} };
		case ETerrainPatchFaceDirection::D: return { {1,0,0}, {0,0,-1}, {0,-1,0} };
		case ETerrainPatchFaceDirection::E: return { {1,0,0}, {0,1,0}, {0,0,1} };
		case ETerrainPatchFaceDirection::F: return { {-1,0,0}, {0,1,0}, {0,0,-1} };
		}
		return { {1,0,0}, {0,1,0}, {0,0,1} };
	}

	constexpr uint32_t maxLOD = 12;     // Maximum depth of the quadtree
	constexpr double lodFactor = 1.75;   // Distance split threshold multiplier


	PlanetDrawer::PlanetDrawer(EngineDevice& device, World& world, const RenderingFormats& formats, VkSampleCountFlagBits samples)
		: device(device), world(world), junkPileMutexes({{},{}})
	{
		// create a planet made up of one node per root face
		planets.push_back(std::make_unique<Planet>());
		Planet& planet = *planets.back();
		planet.resolution = 32;
		planet.radius = 6371 * 100000; // radius of earth
		planet.transform.sector = { 0, 0, 0 };

		// create material
		ShaderFilePaths shaders(makePath("shaders/compiled/planet.vert.spv"), makePath("shaders/compiled/planet.frag.spv"));
		auto layouts = world.getScene().getDescriptorSetLayouts();
		MaterialCreateInfo matInfo(shaders, layouts, samples, formats, sizeof(ShaderPushConstants::EngineMeshPushConstants), EMatSet::NO);
		matInfo.shadingProperties.polygonMode = VK_POLYGON_MODE_LINE;
		matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		planet.material = std::make_shared<Material>(matInfo, device);
		planet.material->finalize();

		for (auto& pile : junkPiles) pile.reserve(1000);
		regenerate(planet);
	}

	PlanetDrawer::~PlanetDrawer() = default;

	std::vector<std::unique_ptr<JunkPileItem>>& PlanetDrawer::getJunkPile(uint32_t threadIndex)
	{
		return junkPiles[(currentFrameIndex + threadIndex) % junkPiles.size()];
	}

	void PlanetDrawer::ensureJunkPileLock(uint32_t threadIndex, std::unique_lock<std::mutex>& lock)
	{
		auto& m = junkPileMutexes[(currentFrameIndex + threadIndex) % junkPiles.size()];
		lock = std::move(std::unique_lock<std::mutex>(m, std::try_to_lock));
		assert(lock.owns_lock() && "unsafe to free from and write to the same junk pile concurrently");
	}

	void PlanetDrawer::regenerate(Planet& planet)
	{
		for (auto& rootPatch : planet.roots)
		{
			// move previous geometry buffers (if present) to be deleted on a later frame
			rootPatch->scheduleFreeBuffers(getJunkPile(0), currentFrameIndex);
		}
		planet.roots.clear();

		// create each root face
		for (uint32_t i = 0; i < 6; i++)
		{
			planet.roots.push_back(std::make_unique<TerrainPatch>(device, planet.resolution, planet.radius));
			TerrainPatch& root = *planet.roots.back();

			// set up quad metadata for root
			root.center = { -1.0f, -1.0f }; // using bottom-left as offset
			root.size = 2.0f; // full extent of the face
			root.lodLevel = 0;
			root.face = static_cast<ETerrainPatchFaceDirection>(i);
			root.generate();
		}

		TerrainPatch& root1 = *planet.roots[1];
		//while (not root1.updateReadiness())
		//{
		//	std::cout << "patch still loading...\n";
		//	continue;
		//}

		
		// test - just split one of the root faces and its children, for now
		//root1.split();
		//for (auto& c : root1.children)
		//{
		//	c->split();
		//	for (auto& j : c->children)
		//	{
		//		j->split();
		//		for (auto& k : j->children)
		//		{
		//			k->split();
		//		}
		//	}
		//}
	}

	void PlanetDrawer::render(VkCommandBuffer commandBuffer, uint32_t frameIndex, const Transform& cameraTransform, double dt)
	{
		currentFrameIndex = frameIndex;
		delta = dt;
		cmdBuffer = commandBuffer;
		camTransform = cameraTransform;
		Scene& scene = world.getScene();
		const auto sets = scene.getDescriptorSets(frameIndex);

		if ((not updater.valid()) || updater.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			// start LOD updates in another thread
			updater = std::async(std::launch::async, &PlanetDrawer::asyncUpdate, this);
		}

		Material* currentMaterial = nullptr;
		for (const auto& planet : planets)
		{
			if (planet->material.get() != currentMaterial)
			{
				// bind planet's material
				currentMaterial = planet->material.get();
				currentMaterial->bindToCommandBuffer(commandBuffer);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentMaterial->getPipelineLayout(),
						0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
			}

			// draw patches
			for (std::unique_ptr<TerrainPatch>& rootPatch : planet->roots)
			{
				attemptReplace(rootPatch);
				drawRecursive(rootPatch, *planet);
			}
		}
	}

	void PlanetDrawer::asyncUpdate()
	{
		cleanJunkPile(currentFrameIndex);

		for (const auto& planet : planets)
		{
			for (std::unique_ptr<TerrainPatch>& patch : planet->roots)
			{
				// update patch and its children
				evaluatePatchLOD(*patch, *planet);
			}
		}
	}

	void PlanetDrawer::evaluatePatchLOD(TerrainPatch& patch, Planet& planet)
	{
		patch.updateLoadState();

		if (shouldSplit(patch, planet))
		{
			// if the patch is a leaf, split it
			if (patch.stateIs(TerrainPatch::EState::LEAF))
			{
				std::unique_lock<std::mutex> lock(updaterMutex); // will block if render thread is performing replacement
				if (not patch.next) patch.splitReplace();
			}
			else
			{
				// recursively update children (skip if patch was just split)
				for (auto& child : patch.children)
				{
					evaluatePatchLOD(*child, planet);
				}
			}
		}
		else if (patch.children.size() > 0)
		{
			// distance is too far, remove geometry from child patches 
			std::unique_lock<std::mutex> lock(updaterMutex);
			if (not patch.next) patch.mergeReplace();
		}
	}

	bool PlanetDrawer::shouldSplit(TerrainPatch& patch, Planet& planet)
	{
		// get 2D center of this patch
		const float halfSize = patch.size * 0.5f;
		const Vec2 localCenter2D =
		{
			patch.center.x + halfSize,
			patch.center.y + halfSize
		};

		// project 2D center to 3D unit sphere direction
		const FaceBasis basis = getFaceBasis(patch.face);

		// apply equiangular warping to match mesh geometry
		constexpr double PI_OVER_4 = 0.78539816339;
		float tan_x = static_cast<float>(std::tan(localCenter2D.x * PI_OVER_4));
		float tan_y = static_cast<float>(std::tan(localCenter2D.y * PI_OVER_4));
		const Vec cubePoint = basis.forward + (basis.right * tan_x) + (basis.up * tan_y);

		const Vec unitDirection = cubePoint.getNormalized();

		// true world-space position of patch center (planet is assumed to be at the center of its own sector)
		const Vec patchCenter = unitDirection * static_cast<float>(planet.radius);

		// true distance to camera
		const Transform patchTransform(patchCenter, Vec(0), Vec(0), planet.transform.sector);

		const double distanceToCamera = Math::calculateDistance(patchTransform, camTransform);

		// estimate physical size of patch on the sphere surface (a root patch of size 2 spans roughly 90 deg / PI half-circumference)
		const double patchArcLength = (static_cast<double>(patch.size) / 2.0) * (std::numbers::pi * 0.5) * planet.radius;

		// check split criteria
		bool should = (distanceToCamera < patchArcLength * lodFactor) && (patch.lodLevel < maxLOD);
		//std::cout << (should ? "/ SPLIT\n" : "\\split not\n");
		return should;
	}


	void PlanetDrawer::drawRecursive(std::unique_ptr<TerrainPatch>& patch, Planet& planet)
	{
		if (patch->isParent())
		{
			for (auto& childPatch : patch->children)
			{
				attemptReplace(childPatch);
				drawRecursive(childPatch, planet);
			}
		}
		else
		{
			drawLeafPatch(patch, planet);
		}
	}

	void PlanetDrawer::attemptReplace(std::unique_ptr<TerrainPatch>& patch)
	{
		{
			std::unique_lock<std::mutex> lock(updaterMutex, std::try_to_lock);
			if (lock.owns_lock() && patch->next)
			{
				// this patch has an updated version ready, replace it
				std::unique_ptr<TerrainPatch> next = std::move(patch->next);

				std::unique_lock<std::mutex> trashLock;
				ensureJunkPileLock(0, trashLock);
				patch->scheduleFreeBuffersRecursive(getJunkPile(0), currentFrameIndex);
				patch->children.clear();

				patch = std::move(next);
			}
		}
	}

	void PlanetDrawer::drawLeafPatch(std::unique_ptr<TerrainPatch>& patch, Planet& planet)
	{
		//std::cout << "distance from center: " << (Math::calculateDistanceToSectorCenter(camTransform, SectorCoord()) * 0.00001) << " km\n";
		if (not patch->stateIs(TerrainPatch::EState::LEAF)) return; // patch geometry is still loading

		// translation and scaling factor
		Vec position;
		double k = 1;

		const Vec64 rel = Math::calculateRelativeCoordsXYZ(camTransform, planet.transform);
		const double distance = rel.getLength();

		if (distance > finalDrawDistance) [[likely]]
		{
			// generate new position relative to player's sector, pinned to a certain distance so it stays closer than the far clip plane
			Vec direction =
			{
				static_cast<float>(rel.x / distance),
				static_cast<float>(rel.y / distance),
				static_cast<float>(rel.z / distance)
			};
			position = camTransform.translation + direction * finalDrawDistance;

			k = finalDrawDistance / distance;
		}
		else [[unlikely]]
		{
			// this usually won't happen unless the radius is tiny, or we go underground
			position = Math::calculateRelativePositionForRendering(planet.transform, camTransform.sector);
		}

		// the unit sphere is scaled up to its final radius in the shader
		const double visualRadius = planet.radius * k;
		const Vec scale = Vec(static_cast<float>(visualRadius));

		ShaderPushConstants::EngineMeshPushConstants push{};
		// TODO: optimize this
		push.transform = EngineCore::cglm::makeMatrixQ(Vec(0), 0, scale, position);
		push.normalMatrix = glm::transpose(glm::inverse(push.transform));

		planet.material->writePushConstants(cmdBuffer, push);
		// record mesh draw command
		patch->draw(cmdBuffer);
	}

	int32_t PlanetDrawer::acquireJunkPile(std::unique_lock<std::mutex>& lock, bool allowFail)
	{
		// try to acquire any available mutex without waiting
		//for (uint32_t i = 0; i < junkPileMutexes.size(); i++)
		//{
		//	std::unique_lock<std::mutex> tryLock(junkPileMutexes[i], std::try_to_lock);
		//	if (tryLock.owns_lock())
		//	{
		//		lock = std::move(tryLock);
		//		return i;
		//	}
		//}
		//if (allowFail) return -1;
		//
		//// if none were free, do a blocking wait
		//lock = std::unique_lock<std::mutex>(junkPileMutexes[0]);
		return 0;
	}

	// do not call this on the exact same frame as marking items to be freed
	void PlanetDrawer::cleanJunkPile(uint32_t cleaningFrameIndex)
	{
		std::unique_lock<std::mutex> trashLock;
		ensureJunkPileLock(1, trashLock);

		auto& junkPile = getJunkPile(1);
		std::erase_if(junkPile, [cleaningFrameIndex](const auto& item)
			{
				return item->freeOnFrameIndex == cleaningFrameIndex;
			});
		//if (junkPile.size()>0) std::cout << (currentFrameIndex + 1) % junkPiles.size() << " junk pile size: " << junkPile.size() << "\n";
	}

}