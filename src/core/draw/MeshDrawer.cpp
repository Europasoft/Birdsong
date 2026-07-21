#include "MeshDrawer.h"

#include "core/gpu/Device.h"
#include "core/engine/Camera.h"
#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/world/Sector.h"
#include "core/nodes/MeshNode.h"
#include "core/types/glm_conversions.h"

#include <stdexcept>
#include <array>
#include <limits>
#include <iostream> // temporary

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace EngineCore
{
	MeshDrawer::~MeshDrawer() = default;

	void MeshDrawer::renderMeshes(VkCommandBuffer commandBuffer, WorldSystem::World& world,
			double deltaTimeSeconds, double time, uint32_t frameIndex, VkDescriptorSet sceneGlobalDescriptorSet, 
			const glm::mat4& viewMatrix) //FakeScaleTest082
	{
		using namespace WorldSystem;

		Scene& scene = world.getScene();
		const SectorCoord cameraSectorCoord = scene.getLocalSectorCoordinate();
		const float S = static_cast<float>(scene.getSectorSize());

		for (Sector* sector : scene.getLoadedSectors())
		{
			for (const Nodes::MeshNode* meshNode : sector->getMeshNodes())
			{
				auto material = meshNode->getMaterial();
				material->bindToCommandBuffer(commandBuffer); // bind material-specific shading pipeline

				std::vector<VkDescriptorSet> sets;
				sets.push_back(sceneGlobalDescriptorSet); // scene global descriptor set

				if (auto* matSet = material->getMaterialSpecificDescriptorSet())
				{
					// bind material-specific descriptor set
					sets.push_back(matSet->getDescriptorSet(frameIndex));
				}

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipelineLayout(),
					0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

				ShaderPushConstants::MeshPushConstants push{};
				const auto& transform = meshNode->getTransform();

				// get the unified world space position relative to the camera's sector origin
				const Vec meshPosRelative = WorldSystem::calculateRelative(transform.translation, sector->coordinates, cameraSectorCoord);
				push.transform = cglm::makeMatrixQ(transform.rotation, transform.rotation_w, transform.scale, meshPosRelative);
				std::cout << "\n rot x: " << transform.rotation.x << " w: " << transform.rotation_w;
				push.normalMatrix = glm::transpose(glm::inverse(push.transform));
				material->writePushConstants(commandBuffer, push);

				// record mesh draw command
				meshNode->bind(commandBuffer);
				meshNode->draw(commandBuffer);
			}
		}

	}

	glm::mat4 MeshDrawer::lerpMat4(float t, glm::mat4 matA, glm::mat4 matB) 
	{
		glm::mat4 matOut{};

		for (int c = 0; c != 4; c++)
		{
			for (int r = 0; r != 4; r++)
			{
				matOut[c][r] = lerp(matA[c][r], matB[c][r], t);
			}
		}

		return matOut;
	}

}