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
		auto& sectors = world.getScene().getLoadedSectors();
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
				

				/*if (camera != nullptr)
			{
				// camera rotation
				//camera->transform.rotation += glm::vec3{ -x, y, 0.0 } * 0.03f;
				glm::vec3 rot = { -inputSysPtr->getMouseDelta().x, inputSysPtr->getMouseDelta().y, 0.f};
				rot = { Transform3D::degToRad(rot.x), Transform3D::degToRad(rot.y), 0.f };
				camera->transform.rotation += rot * 0.03f;
				auto x = camera->transform.rotation.x; auto y = camera->transform.rotation.y; auto z = camera->transform.rotation.z;
				std::cout << "x: " << x << " y: " << y << " z: " << z << "\n \n \n";

				// camera translation
				glm::vec3 camFwdVec = camera->transform.getForwardVector();
				float fwdInput = inputSysPtr->getAxisValue(0);
				float constexpr epsilon = std::numeric_limits<float>::epsilon();
				if ((glm::dot(camFwdVec, camFwdVec) > epsilon) && (fwdInput > epsilon || fwdInput < -epsilon))
				{
					camera->transform.translation += glm::normalize(camFwdVec) * (fwdInput * 1.2f * deltaTimeSeconds);
				}
				camera->transform.translation.y += -inputSysPtr->getAxisValue(1) * deltaTimeSeconds * 1.2f;
				camera->transform.translation.z += inputSysPtr->getAxisValue(2) * deltaTimeSeconds * 1.2f;
			}
			else 
			{ throw std::runtime_error("renderEngineObjects null camera pointer"); }*/


				/* old way of sending matrices to gpu
			push.transform = projectionMatrix * worldMatrix * viewMatrix * meshMatrix;
			vkCmdPushConstants(commandBuffer, mesh->getMaterial()->getPipelineLayout(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(SimplePushConstantData), &push);*/

				//FakeScaleTest082
				/*if (mesh->useFakeScale)
				{
					ShaderPushConstants::MeshPushConstants push{};
					push.transform = fakeScaleOffsets.mat4();
					material->writePushConstants(commandBuffer, push);
				} 
				else */
				{
					// NON-TEST CODE!
					ShaderPushConstants::MeshPushConstants push{};
					push.transform = mesh->getTransform().mat4();
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