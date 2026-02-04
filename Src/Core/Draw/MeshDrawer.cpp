#include "MeshDrawer.h"

#include "Core/GPU/Device.h"
#include "Core/Camera.h"
#include "Core/WorldSystem/World.h"

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

			// this offset moves the position from sector-local to camera-local space
			// by accounting for the distance between the camera's sector and the object's sector
			glm::vec3 sectorOffset = {
				static_cast<float>(sector->coordinates.x - cameraSector.x) * S,
				static_cast<float>(sector->coordinates.y - cameraSector.y) * S,
				static_cast<float>(sector->coordinates.z - cameraSector.z) * S
			};

			for (uint32_t i = 0; i < meshes.size(); i++)
			{
				auto& mesh = meshes[i];
				auto material = mesh->getMaterial();

				material->bindToCommandBuffer(commandBuffer); // bind material-specific shading pipeline

				std::vector<VkDescriptorSet> sets;
				// scene global descriptor set
				sets.push_back(sceneGlobalDescriptorSet);

				if (auto* matSet = material->getMaterialSpecificDescriptorSet())
				{
					// bind material-specific descriptor set
					sets.push_back(matSet->getDescriptorSet(frameIndex));
				}

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipelineLayout(),
										0, sets.size(), sets.data(), 0, nullptr);

				// spin 3D primitive - demo
				if (s == 1 && i == 0)
				{
					float spinRate = 0.1f;
					mesh->getTransform().rotation.z = glm::mod(mesh->getTransform().rotation.z + spinRate * deltaTimeSeconds, glm::two_pi<float>());
					mesh->getTransform().rotation.y = glm::mod(mesh->getTransform().rotation.y + spinRate * 0.8f * deltaTimeSeconds, glm::two_pi<float>());
					continue; // TODO: this skips rendering the first mesh!!!
				}

				// spin 3D primitive - demo
				if (s == 1 && i == 1)
				{

					float spinRate = 0.3f;
					mesh->getTransform().rotation.z = glm::mod(mesh->getTransform().rotation.z + spinRate * deltaTimeSeconds, glm::two_pi<float>());
				}
				
				{
					// NON-TEST CODE!
					ShaderPushConstants::MeshPushConstants push{};
					const auto& transform = mesh->getTransform();
					push.transform = Transform::makeMatrix(transform.rotation, transform.scale, transform.translation + sectorOffset);
					push.normalMatrix = glm::transpose(glm::inverse(push.transform));
					material->writePushConstants(commandBuffer, push);
				}

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