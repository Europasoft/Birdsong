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
		VkCommandBuffer commandBuffer;
		uint32_t frameIndex;
		uint32_t instanceID;
		VkDeviceAddress instanceBufferAddress;
		std::vector<VkDescriptorSet> descriptorSets;
	};

	void MeshDrawer::renderMeshes(VkCommandBuffer commandBuffer, WorldSystem::World& world, uint32_t frameIndex)
	{
		using namespace WorldSystem;

		Scene& scene = world.getScene();

		uint32_t instanceID = 0;
		for (Sector* sector : scene.getLoadedSectors())
		{
			for (EngineNodeData* nodeData : sector->nodes().getMeshes())
			{
				WorldSystem::Mesh& mesh = *nodeData->mesh.get();
				DrawMeshContext ctx
				{
						mesh, commandBuffer,
						frameIndex, instanceID,
						scene.getInstanceBuffer().getDeviceAddress(frameIndex),
						scene.getDescriptorSets(frameIndex)
				};
				renderOne(ctx);
				instanceID++;
			}
		}

	}

	void MeshDrawer::renderOne(DrawMeshContext& ctx)
	{
		Material& material = *ctx.mesh.getMaterial().get();
		material.bindToCommandBuffer(ctx.commandBuffer); // bind material-specific shading pipeline

		if (material.hasDescriptorSet())
		{
			// bind material-specific descriptor set
			auto& matSet = material.getDescriptorSet();
			ctx.descriptorSets.push_back(matSet.getDescriptorSet(ctx.frameIndex));
		}

		vkCmdBindDescriptorSets(ctx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.getPipelineLayout(),
			0, static_cast<uint32_t>(ctx.descriptorSets.size()), ctx.descriptorSets.data(), 0, nullptr);

		ShaderPushConstants::MeshPushConstants push{};
		push.instanceBufferAddress = ctx.instanceBufferAddress;
		push.instanceID = ctx.instanceID;

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