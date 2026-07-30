#include "core/draw/planets/PlanetDrawer.h"
#include "core/draw/planets/TerrainPatch.h"
#include "core/gpu/Device.h"
#include "core/gpu/Material.h"
#include "core/include/shared/Transform.h"
#include "core/types/glm_conversions.h"

#include "core/engine/Camera.h"
#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/world/Sector.h"
//#include "core/draw/planets/LODSphere.h"

#include <stdexcept>
#include <array>
#include <limits>

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>

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
		matInfo.shadingProperties.polygonMode = VK_POLYGON_MODE_FILL;
		matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		planet.material = std::make_shared<Material>(matInfo, device);
		planet.material->finalize();

		regenerate(planet);
	}

	void PlanetDrawer::regenerate(Planet& planet)
	{
		vkDeviceWaitIdle(device.device()); // STRICTLY TEMPORARY
		planet.roots.clear();
		vkDeviceWaitIdle(device.device()); // STRICTLY TEMPORARY

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

			planet.centerTransform.translation.x = currentXoffset;
			planet.centerTransform.sector = { 0, 0, 0 };
			planet.centerTransform.scale = { 1 };
		}

		// test - just split one of the root faces and its children, for now
		TerrainPatch& root1 = *planet.roots[1];
		root1.split();
		for (auto& c : root1.children)
		{
			c->split();
			for (auto& j : c->children)
			{
				j->split();
				for (auto& k : j->children)
				{
					k->split();
				}
			}
		}
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
		delta = dt;
		cmdBuffer = commandBuffer;
		camTransform = cameraTransform;
		Scene& scene = world.getScene();
		const auto sets = scene.getDescriptorSets(frameIndex);
		
		// experiment: grow and move further away
		/*if (tempTimer < 900)
		{
			tempTimer -= delta;
		}
		if (tempTimer <= 0)
		{
			const float growBy = 100 * 1000 * 5000;
			rr.radius += growBy;
			currentXoffset += growBy;
			//if ((rr.radius * 0.00001) >= 80000) { rr.radius = 100 * 1000 * 80000; }
		
			tempTimer = 0.6;
			regenerate();
		
			if ((rr.radius * 0.00001) >= 80000) { tempTimer = 999; }
			std::cout << "radius: " << rr.radius * 0.00001 << " km\n";
		}*/

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
		if (not patch.isLeafNode())
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
		const Transform& t = planet.centerTransform;
		// calculate position relative to player's sector
		const Vec meshPosRelative = WorldSystem::calculateRelative(t.translation, t.sector, world.getScene().getLocalSectorCoordinate());

		ShaderPushConstants::EngineMeshPushConstants push{};
		push.transform = EngineCore::cglm::makeMatrixQ(t.rotation, t.rotation_w, t.scale, meshPosRelative);
		push.normalMatrix = glm::transpose(glm::inverse(push.transform));

		planet.material->writePushConstants(cmdBuffer, push);
		// record mesh draw command
		patch.bindAndDraw(cmdBuffer);
	}

}