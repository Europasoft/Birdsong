#include "MeshDrawer.h"

#include "core/gpu/Device.h"
#include "core/engine/Camera.h"
#include "core/world/World.h"
#include "core/world/Sector.h"

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
	void MeshDrawer::renderMeshes(VkCommandBuffer commandBuffer, WorldSystem::World& world,
			const float& deltaTimeSeconds, float time, uint32_t frameIndex, VkDescriptorSet sceneGlobalDescriptorSet, 
			const glm::mat4& viewMatrix) //FakeScaleTest082
	{
		auto& scene = world.getScene();
		auto& sectors = scene.getLoadedSectors();
		const auto& cameraSector = scene.getLocalSectorCoordinate();
		const float S = static_cast<float>(scene.getSectorSize());

		for (uint32_t s = 0; s < sectors.size(); s++)
		{
			auto& sector = sectors[s];
			auto& meshes = sector->primitives;
			if (sector->isCulled)
				continue;

			for (uint32_t i = 0; i < meshes.size(); i++)
			{
				auto& mesh = meshes[i];
				auto material = mesh->getMaterial();

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
				const auto& transform = mesh->getTransform();
				// get the unified world space position relative to the camera's sector origin
				const Vec meshPosRelative = WorldSystem::calculateRelative(transform.translation, sector->coordinates, cameraSector);
				push.transform = Transform::makeMatrix(transform.rotation, transform.scale, meshPosRelative);
				push.normalMatrix = glm::transpose(glm::inverse(push.transform));
				material->writePushConstants(commandBuffer, push);

				// record mesh draw command
				mesh->bind(commandBuffer);
				mesh->draw(commandBuffer);
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