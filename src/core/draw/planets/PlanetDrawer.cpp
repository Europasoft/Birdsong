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

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
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
		planet.radius = 100;

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

	void PlanetDrawer::regenerate(Planet& planet)
	{
		for (auto& terrainPatch : planet.roots)
		{
			// move previous geometry buffers (if present) to be deleted on a later frame
			junkPile.push_back(std::make_unique<JunkPileItem>(std::move(terrainPatch), currentFrameIndex));
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

			root.generateGeometry();
			root.geometryToGPU();

			//planet.centerTransform.translation.x = currentXoffset;
			planet.transform.sector = { 0, 0, 0 };
		}

		TerrainPatch& root1 = *planet.roots[1];
		//while (not root1.updateReadiness())
		//{
		//	std::cout << "patch still loading...\n";
		//	continue;
		//}

		EngineClock clock{};
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
		std::cout << "====================== split all took " << clock.getElapsed() << " seconds\n";
	}

	/*void PlanetDrawer::updateLOD(Quad& quad, const Vec64& cameraPos, std::shared_ptr<Material> material, ResRad& r)
	{
		// 1. Estimate 3D center point of this quad on the sphere surface
		// (You can map quad.center + quad.size * 0.5f from local face space to 3D world space)
		Vec264 localCenter2D = { quad.center.x + quad.size * 0.5f, quad.center.y + quad.size * 0.5f };
		Vec64 sphereCenter3D = Planets::projectToSphere(static_cast<uint32_t>(quad.face), localCenter2D, r.radius);

		// Account for planet scale/transform offset if necessary
		// float distance = distance(cameraPos, sphereCenter3D * transformScale + transformTranslation);
		float distance = Vec64::distance(cameraPos, sphereCenter3D);

		// 2. Define your split threshold rule (e.g., split if node size relative to distance is large)
		// Adjust 'splitThreshold' multiplier to tune when nodes subdivide
		float splitThreshold = quad.size * 2.5f;
		bool shouldSplit = (distance < splitThreshold) && (quad.lodLevel < 6); // Max LOD cap e.g. 6

		if (shouldSplit)
		{
			if (quad.node) // It's currently a leaf node, split it!
			{
				splitQuad(quad, material, r);
			}

			// Recursively evaluate children
			for (auto& child : quad.children)
			{
				updateLOD(*child, cameraPos, material, r);
			}
		}
		else
		{
			if (!quad.node) // It has children, but we are far enough to merge back
			{
				// Check if all children are leaves before merging (prevents popping artifacts)
				bool allChildrenAreLeaves = true;
				for (const auto& child : quad.children)
				{
					if (!child->node)
					{
						allChildrenAreLeaves = false; break;
					}
				}

				if (allChildrenAreLeaves)
				{
					// Re-create parent node mesh/leaf state and clear children
					quad.node = std::make_unique<EngineNodeData>(nullptr, device);
					quad.node->mesh = std::make_unique<Mesh>(device);
					quad.node->mesh->build(Planets::generateSubFace(static_cast<int>(quad.face), r.resolution, r.radius, { quad.center.x, quad.center.y }, quad.size, quad.lodLevel, true).toSinglePrecision());

					// Copy transform from children/planet context
					quad.node->engineTransform = quad.children[0]->node->engineTransform;
					quad.node->mesh->setMaterial(material);

					quad.children.clear();
				}
			}
		}
	}*/

	/*void PlanetDrawer::mergeQuad(Quad& quad)
	{
		quad.children.clear();
		// re-instantiate quad.node here if you want it to be a leaf again, 
		// or handle it inside your LOD evaluation loop.
	}*/

	PlanetDrawer::~PlanetDrawer() = default;

	void PlanetDrawer::render(VkCommandBuffer commandBuffer, uint32_t frameIndex, const Transform& cameraTransform, double dt)
	{
		currentFrameIndex = frameIndex;
		delta = dt;
		cmdBuffer = commandBuffer;
		camTransform = cameraTransform;
		Scene& scene = world.getScene();
		const auto sets = scene.getDescriptorSets(frameIndex);

		cleanJunkPile(); // called at the start, ensures that we don't delete any of the geometry created this exact frame

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
		if (not patch.updateReadiness()) return;

		const Vec64 rel = Math::calculateRelativeCoordsXYZ(camTransform, planet.transform);
		const double distance = rel.getLength();
		Vec position;
		if (distance < finalDrawDistance) [[unlikely]]
		{
			// this usually won't happen, unless the "planet" is tiny, or we go underground
			position = Math::calculateRelativePositionForRendering(planet.transform, camTransform.sector);
		}
		else [[likely]]
		{
			// generate new position relative to player's sector, pinned to a certain distance so it stays closer than the far clip plane
			Vec direction =
			{
				static_cast<float>(rel.x / distance),
				static_cast<float>(rel.y / distance),
				static_cast<float>(rel.z / distance)
			};
			position = camTransform.translation + direction * finalDrawDistance;
		}

		ShaderPushConstants::EngineMeshPushConstants push{};
		push.transform = EngineCore::cglm::makeMatrixQ(Vec(0), 0, 1, position);
		push.normalMatrix = glm::transpose(glm::inverse(push.transform));

		planet.material->writePushConstants(cmdBuffer, push);
		// record mesh draw command
		patch.draw(cmdBuffer);
	}

	void PlanetDrawer::split(std::unique_ptr<TerrainPatch>& patch)
	{
		// call stealBuffers after TerrainPatch::split(), so they can be freed at the right time (on a later frame)
		//if (patch->split())
	}

	// do not call this on the exact same frame as marking items to be freed
	void PlanetDrawer::cleanJunkPile()
	{
		for (size_t i = 0; i < junkPile.size(); i++)
		{
			auto& item = junkPile[i];
			// the frame index has looped back around to the same index with which the object was created, meaning it is safe to deallocate
			if (item->freeOnFrameIndex == currentFrameIndex)
			{
				junkPile.erase(junkPile.begin() + i);
				std::cout << "removed old patch\n";
			}
		}
		// check if any split nodes have stale geometry buffers to free
		for (const auto& planet : planets)
		{
			for (const auto& rootPtr : planet->roots)
			{
				recursiveFree(*rootPtr);
			}
		}
	}

	void PlanetDrawer::recursiveFree(TerrainPatch& patch)
	{
		if (patch.canFreeOnFrame(currentFrameIndex))
			patch.freeBuffers();
		for (auto& childPatch : patch.children)
		{
			recursiveFree(*childPatch);
		}
	}

	//double PlanetDrawer::calculateAxisDisplacement(float startLocalOffset, SectorInt startSector, SectorInt targetSector)
	//{
	//	const SectorInt sectorDelta = targetSector - startSector;
	//	const double distanceDelta = static_cast<double>(sectorDelta) * static_cast<double>(Sector::SECTOR_SIZE);
	//	const double positionA = static_cast<double>(startLocalOffset) + distanceDelta;
	//	return -positionA;
	//};

	//Vec PlanetDrawer::getDirectionToPlanet(const Transform& startTransform, const SectorCoord& planetSector) const
	//{
	//	// calculate high-precision displacement vector along each axis
	//	const double dx = calculateAxisDisplacement(startTransform.translation.x, startTransform.sector.x, planetSector.x);
	//	const double dy = calculateAxisDisplacement(startTransform.translation.y, startTransform.sector.y, planetSector.y);
	//	const double dz = calculateAxisDisplacement(startTransform.translation.z, startTransform.sector.z, planetSector.z);
	//
	//	// compute 3D magnitude in double precision
	//	const double lengthSq = dx * dx + dy * dy + dz * dz;
	//
	//	if (lengthSq == 0.0)
	//	{
	//		return Vec{ 0 }; // handle zero-distance edge case
	//	}
	//
	//	// normalize in double precision, then cast down to float direction
	//	const double invLength = 1.0 / std::sqrt(lengthSq);
	//	return Vec
	//	{
	//		static_cast<float>(dx * invLength),
	//		static_cast<float>(dy * invLength),
	//		static_cast<float>(dz * invLength)
	//	};
	//}

	//double PlanetDrawer::getDistanceToSectorCenter(const Transform& startTransform, const SectorCoord& targetSector)
	//{
	//	const double dx = calculateAxisDisplacement(startTransform.translation.x, startTransform.sector.x, targetSector.x);
	//	const double dy = calculateAxisDisplacement(startTransform.translation.y, startTransform.sector.y, targetSector.y);
	//	const double dz = calculateAxisDisplacement(startTransform.translation.z, startTransform.sector.z, targetSector.z);
	//
	//	// 2. Compute hypotenuse using std::hypot to prevent overflow/underflow during squaring
	//	return std::hypot(dx, dy, dz);
	//}



}