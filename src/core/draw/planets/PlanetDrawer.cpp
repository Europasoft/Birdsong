#include "core/draw/planets/PlanetDrawer.h"

#include "core/gpu/Device.h"
#include "core/engine/Camera.h"
#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/world/Sector.h"
#include "core/nodes/EMesh.h"
#include "core/include/shared/Transform.h"
#include "core/types/glm_conversions.h"
#include "core/gpu/descriptors/InstanceBuffer.h"
#include "core/nodes/EngineNodeData.h"

#include "core/draw/planets/LODSphere.h"
#include "core/gpu/Material.h"

#include <stdexcept>
#include <array>
#include <limits>

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

static constexpr auto presetResolution = 2;
static constexpr auto presetRadius = 1000.f;

namespace EngineCore
{
	using namespace WorldSystem;
	using namespace EngineInterface;

	PlanetDrawer::PlanetDrawer(EngineDevice& device, World& world, const RenderingFormats& formats, VkSampleCountFlagBits samples)
		: device(device), world(world)
	{
		// create a planet made up of one node per root face
		planets.push_back(std::make_unique<PlanetNodeContext>());
		PlanetNodeContext& ctx = *planets.back();

		// create material
		ShaderFilePaths shaders(makePath("shaders/compiled/planet.vert.spv"), makePath("shaders/compiled/planet.frag.spv"));
		auto layouts = world.getScene().getDescriptorSetLayouts();
		MaterialCreateInfo matInfo(shaders, layouts, samples, formats, sizeof(ShaderPushConstants::EngineMeshPushConstants), EMatSet::NO);
		matInfo.shadingProperties.polygonMode = VK_POLYGON_MODE_FILL;
		matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		ctx.material = std::make_shared<Material>(matInfo, device);
		ctx.material->finalize();

		// create each root face
		uint32_t i = 0;
		for (uint32_t i = 0; i < 6; i++)
		{
			ctx.rootFaces.push_back(std::make_unique<Quad>());
			Quad& quad = *ctx.rootFaces.back();

			// set up quad metadata for root
			quad.center = { -1.0f, -1.0f }; // using bottom-left as offset
			quad.size = 2.0f;             // full extent of the face
			quad.lodLevel = 0;
			quad.face = static_cast<FaceDirection>(i);

			quad.node = std::make_unique<EngineNodeData>(nullptr, device);
			quad.node->mesh = std::make_unique<Mesh>(device);

			// generate the mesh
			quad.node->mesh->build(Planets::generateSubFace(i, presetResolution, presetRadius, {quad.center.x, quad.center.y}, quad.size, true));

			Transform tf{};
			tf.translation.x = 800;
			tf.scale = { 1 };
			tf.sector = { 0, 0, 0 };
			quad.node->engineTransform = tf;
			quad.node->mesh->setMaterial(ctx.material);
		}

		// test - just split one of the root faces and its children, for now
		auto& rootface = *ctx.rootFaces[1];
		splitQuad(rootface, ctx.material, presetRadius, presetResolution);
		for (auto& c : rootface.children)
		{
			splitQuad(*c, ctx.material, presetRadius, presetResolution);
			for (auto& j : c->children)
			{
				splitQuad(*j, ctx.material, presetRadius, presetResolution);
			}
		}
	}

	void PlanetDrawer::updateLOD(Quad& quad, const Vec64& cameraPos, std::shared_ptr<Material> material, float radius, int resolution)
	{
		// 1. Estimate 3D center point of this quad on the sphere surface
		// (You can map quad.center + quad.size * 0.5f from local face space to 3D world space)
		Vec264 localCenter2D = { quad.center.x + quad.size * 0.5f, quad.center.y + quad.size * 0.5f };
		Vec64 sphereCenter3D = Planets::projectToSphere(static_cast<uint32_t>(quad.face), localCenter2D, radius);

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
				splitQuad(quad, material, radius, resolution);
			}

			// Recursively evaluate children
			for (auto& child : quad.children)
			{
				updateLOD(*child, cameraPos, material, radius, resolution);
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
					quad.node->mesh->build(Planets::generateSubFace(static_cast<int>(quad.face), resolution, radius, { quad.center.x, quad.center.y }, quad.size, true));

					// Copy transform from children/planet context
					quad.node->engineTransform = quad.children[0]->node->engineTransform;
					quad.node->mesh->setMaterial(material);

					quad.children.clear();
				}
			}
		}
	}

	void PlanetDrawer::splitQuad(Quad& quad, std::shared_ptr<Material> material, float radius, int resolution)
	{
		if (not quad.node) return; // already split

		float child_size = quad.size * 0.5f;
		Vector2D<float> o = quad.center;

		// 4 local sub-quadrants
		Vector2D<float> offsets[4] = {
			{ o.x,               o.y               }, // bottom-left
			{ o.x + child_size,  o.y               }, // bottom-right
			{ o.x,               o.y + child_size  }, // top-left
			{ o.x + child_size,  o.y + child_size  }  // top-right
		};

		quad.children.resize(4);
		for (int i = 0; i < 4; ++i)
		{
			quad.children[i] = std::make_unique<Quad>();
			Quad& child = *quad.children[i];

			child.center = offsets[i];
			child.size = child_size;
			child.lodLevel = quad.lodLevel + 1;
			child.face = quad.face;

			// build child mesh
			child.node = std::make_unique<EngineNodeData>(nullptr, device);
			child.node->mesh = std::make_unique<Mesh>(device);
			child.node->mesh->build(Planets::generateSubFace(static_cast<int>(child.face), resolution, radius, { child.center.x, child.center.y }, child.size));

			// copy transform properties from parent
			child.node->engineTransform = quad.node->engineTransform;
			child.node->mesh->setMaterial(material);
		}

		// free parent mesh data so it stops rendering and acts as an inner node
		quad.node.reset();
	}

	void PlanetDrawer::mergeQuad(Quad& quad)
	{
		quad.children.clear();
		// re-instantiate quad.node here if you want it to be a leaf again, 
		// or handle it inside your LOD evaluation loop.
	}

	PlanetDrawer::~PlanetDrawer() = default;

	void PlanetDrawer::render(VkCommandBuffer commandBuffer, uint32_t frameIndex)
	{
		Scene& scene = world.getScene();
		const auto sets = scene.getDescriptorSets(frameIndex);

		for (const auto& planetPtr : planets)
		{
			PlanetNodeContext& planet = *planetPtr;
			planet.material->bindToCommandBuffer(commandBuffer);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, planet.material->getPipelineLayout(),
				0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

			for (const auto& quad : planet.rootFaces)
			{
				recurseDrawQuad(*quad, *planet.material, commandBuffer);
			}
		}
	}

	void PlanetDrawer::recurseDrawQuad(Quad& quad, Material& material, VkCommandBuffer commandBuffer)
	{
		if (not quad.node)
		{
			for (auto& subQuad : quad.children)
			{
				recurseDrawQuad(*subQuad, material, commandBuffer);
			}
		}
		else
		{
			drawLeaf(*quad.node, material, commandBuffer);
		}
	}

	void PlanetDrawer::drawLeaf(EngineNodeData& leaf, Material& material, VkCommandBuffer commandBuffer)
	{
		const Transform& t = leaf.engineTransform;
		const Vec meshPosRelative = WorldSystem::calculateRelative(t.translation, t.sector, world.getScene().getLocalSectorCoordinate());

		ShaderPushConstants::EngineMeshPushConstants push{};
		push.transform = EngineCore::cglm::makeMatrixQ(t.rotation, t.rotation_w, t.scale, meshPosRelative);
		push.normalMatrix = glm::transpose(glm::inverse(push.transform));

		material.writePushConstants(commandBuffer, push);
		// record mesh draw command
		leaf.mesh->bind(commandBuffer);
		leaf.mesh->draw(commandBuffer);
	}

}