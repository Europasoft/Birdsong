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
#include <thread>
#include <iostream>
#include "core/engine/EngineClock.h"

namespace EngineCore
{
	using namespace WorldSystem;

	PlanetDrawer::PlanetDrawer(EngineDevice& device, World& world, const RenderingFormats& formats, VkSampleCountFlagBits samples)
		: device(device), world(world)
	{
		// create a planet made up of one node per root face
		planets.push_back(std::make_unique<Planet>());
		Planet& planet = *planets.back();
		planet.resolution = 8;
		planet.radius = 6371 * 100000; // radius of earth

		// create material
		ShaderFilePaths shaders(makePath("shaders/compiled/planet.vert.spv"), makePath("shaders/compiled/planet.frag.spv"));
		auto layouts = world.getScene().getDescriptorSetLayouts();
		MaterialCreateInfo matInfo(shaders, layouts, samples, formats, sizeof(ShaderPushConstants::EngineMeshPushConstants), EMatSet::NO);
		matInfo.shadingProperties.polygonMode = VK_POLYGON_MODE_LINE;
		matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		planet.material = std::make_shared<Material>(matInfo, device);
		planet.material->finalize();

		regenerate(planet);
	}

	PlanetDrawer::~PlanetDrawer() = default;

	void PlanetDrawer::regenerate(Planet& planet)
	{
		for (auto& rootPatch : planet.roots)
		{
			// move previous geometry buffers (if present) to be deleted on a later frame
			rootPatch->scheduleFreeBuffers(junkPile, currentFrameIndex);
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

			//planet.centerTransform.translation.x = currentXoffset;
			planet.transform.sector = { 0, 0, 0 };
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

		cleanJunkPile(); // called at the start, ensures that we don't delete any of the geometry created this exact frame

		updateLOD(); // recursively split/merge patches based on distance

		for (const auto& planet : planets)
		{
			// bind planet's material
			planet->material->bindToCommandBuffer(commandBuffer);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, planet->material->getPipelineLayout(),
				0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

			// draw all patches in the quadtree
			for (const auto& rootPtr : planet->roots)
			{
				TerrainPatch& rootPatch = *rootPtr;
				drawRecursive(rootPatch, *planet);
			}
		}
	}

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

	constexpr uint32_t maxLOD = 32;     // Maximum depth of the quadtree
	constexpr double lodFactor = 1.75;   // Distance split threshold multiplier

	void PlanetDrawer::updateLOD()
	{
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
		EngineClock clock{};
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
		const bool shouldSplit = (distanceToCamera < patchArcLength * lodFactor) && (patch.lodLevel < maxLOD);

		const float ms = clock.getElapsed() * 1000;
		//if (ms > 0.01) std::cout << "============= LOD eval done in " << ms << " ms =============\n";

		if (shouldSplit)
		{
			// if the patch is a leaf, split it
			if (patch.getState() == TerrainPatch::EState::LEAF)
			{
				patch.split(junkPile, currentFrameIndex);
			}
			else
			{
				// recursively update children (probably ok to skip if patch was just split, children can be updated next iteration)
				for (auto& child : patch.children)
				{
					evaluatePatchLOD(*child, planet);
				}
			}
		}
		else
		{
			// distance is too far, reduce geometry
			if (patch.children.size() > 0)
			{
				// remove geometry for child patches
				for (auto& child : patch.children)
				{
					child->scheduleFreeBuffersRecursive(junkPile, currentFrameIndex);
				}
				patch.children.clear();
				// regenerate this patch as a leaf node
				patch.generate();
			}
		}
	}

	void PlanetDrawer::drawRecursive(TerrainPatch& patch, Planet& planet)
	{
		if (patch.isParent())
		{
			for (auto& childPatch : patch.children)
			{
				drawRecursive(*childPatch, planet);
			}
		}
		else
		{
			drawLeafPatch(patch, planet);
		}
	}

	void PlanetDrawer::drawLeafPatch(TerrainPatch& patch, Planet& planet)
	{
		//std::cout << "distance from center: " << (Math::calculateDistanceToSectorCenter(camTransform, SectorCoord()) * 0.00001) << " km\n";
		if (not patch.updateReadiness()) return;

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
		push.transform = EngineCore::cglm::makeMatrixQ(Vec(0), 0, scale, position);
		push.normalMatrix = glm::transpose(glm::inverse(push.transform));

		planet.material->writePushConstants(cmdBuffer, push);
		// record mesh draw command
		patch.draw(cmdBuffer);
	}

	// do not call this on the exact same frame as marking items to be freed
	void PlanetDrawer::cleanJunkPile()
	{
		//if (junkPile.size()) std::cout << "junk pile has " << junkPile.size() << " items\n";
		for (size_t i = 0; i < junkPile.size(); i++)
		{
			JunkPileItem& item = *junkPile[i];
			// the frame index has looped back around to the same index with which the object was created, meaning it is safe to deallocate
			if (item.freeOnFrameIndex == currentFrameIndex)
			{
				junkPile.erase(junkPile.begin() + i);
			}
		}
	}

}