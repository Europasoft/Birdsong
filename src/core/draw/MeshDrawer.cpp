#include "MeshDrawer.h"

#include "core/gpu/Device.h"
#include "core/engine/Camera.h"
#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/world/Sector.h"
#include "core/world/NodeContainer.h"
#include "core/nodes/EMesh.h"
#include "core/include/shared/Transform.h"
#include "core/types/glm_conversions.h"
#include "core/gpu/descriptors/InstanceBuffer.h"

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
	MeshDrawer::~MeshDrawer() = default;

	struct MeshDrawer::DrawMeshContext
	{
		WorldSystem::Mesh& mesh;
		const Transform& transform;
		VkDescriptorSet sceneGlobalDescriptorSet;
		VkCommandBuffer commandBuffer;
		uint32_t frameIndex;
		WorldSystem::SectorCoord cameraSectorCoord;
		uint32_t instanceID;
		VkDeviceAddress instanceBufferAddress;
	};

	void MeshDrawer::renderMeshes(VkCommandBuffer commandBuffer, WorldSystem::World& world,
			double deltaTimeSeconds, double time, uint32_t frameIndex, VkDescriptorSet sceneGlobalDescriptorSet, 
			const glm::mat4& viewMatrix) //FakeScaleTest082
	{
		using namespace WorldSystem;

		Scene& scene = world.getScene();
		
		const float S = static_cast<float>(scene.getSectorSize());

		uint32_t instanceID = 0;
		for (Sector* sector : scene.getLoadedSectors())
		{
			for (EngineNodeData* nodeData : sector->nodes().getMeshes())
			{
				// update the engine-side node transform, using data from game
				nodeData->updateTransformFromGame();
				WorldSystem::Mesh& mesh = *nodeData->mesh.get();
				const Transform& transform = nodeData->engineTransform;
				
				renderOne(DrawMeshContext{
						mesh, transform, sceneGlobalDescriptorSet, commandBuffer, 
						frameIndex, scene.getLocalSectorCoordinate(), instanceID,
						scene.getInstanceBuffer().getDeviceAddress(frameIndex)
					});
				instanceID++;
			}
		}

	}

	void MeshDrawer::renderOne(const DrawMeshContext& ctx)
	{
		Material& material = *ctx.mesh.getMaterial().get();
		material.bindToCommandBuffer(ctx.commandBuffer); // bind material-specific shading pipeline

		std::vector<VkDescriptorSet> sets;
		sets.push_back(ctx.sceneGlobalDescriptorSet); // scene global descriptor set

		if (material.hasDescriptorSet())
		{
			// bind material-specific descriptor set
			auto& matSet = material.getDescriptorSet();
			sets.push_back(matSet.getDescriptorSet(ctx.frameIndex));
		}

		vkCmdBindDescriptorSets(ctx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.getPipelineLayout(),
			0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

		ShaderPushConstants::MeshPushConstants push{};
		push.instanceBufferAddress = ctx.instanceBufferAddress;
		push.instanceID = ctx.instanceID;

		// get the unified world space position relative to the camera's sector origin
		//const Vec meshPosRelative = WorldSystem::calculateRelative(ctx.transform.translation, ctx.transform.sector, ctx.cameraSectorCoord);
		//push.transform = cglm::makeMatrixQ(ctx.transform.rotation, ctx.transform.rotation_w, ctx.transform.scale, meshPosRelative);
		////std::cout << "\n rot x: " << transform.rotation.x << " w: " << transform.rotation_w;
		//push.normalMatrix = glm::transpose(glm::inverse(push.transform));
		material.writePushConstants(ctx.commandBuffer, push);

		// record mesh draw command
		ctx.mesh.bind(ctx.commandBuffer);
		ctx.mesh.draw(ctx.commandBuffer);
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