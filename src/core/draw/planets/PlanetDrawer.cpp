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
		matInfo.shadingProperties.polygonMode = VK_POLYGON_MODE_LINE;
		matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		ctx.material = std::make_shared<Material>(matInfo, device);
		ctx.material->finalize();

		// create each root face
		uint32_t i = 0;
		for (uint32_t i = 0; i < 6; i++)
		{
			ctx.rootFaces.push_back(std::make_unique<Quad>());
			Quad& quad = *ctx.rootFaces.back();
			initRootFaceAsLeaf(quad, i, ctx.material);
		}
	
	}

	void PlanetDrawer::initRootFaceAsLeaf(Quad& quad, uint32_t i, std::shared_ptr<Material> material)
	{
		// this is only used if we want the 6 root faces to be leaf nodes themselves, with no children
		quad.node = std::make_unique<EngineNodeData>(nullptr, device);
		quad.node->mesh = std::make_unique<Mesh>(device);
		quad.node->mesh->build(Planets::generateCubeFace(i, 2, 60));
		Transform tf{};
		tf.translation.x = 65000;
		tf.scale = { 500 };
		tf.sector = { 0, 0, 0 };
		quad.node->engineTransform = tf;
		quad.node->mesh->setMaterial(material);
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