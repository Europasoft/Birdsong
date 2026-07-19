#include "core/draw/DebugDrawer.h"
#include "core/nodes/MeshNode.h"
#include "core/engine/MeshData.h"
#include "core/world/World.h"
#include "core/gpu/Descriptors.h"
#include "core/gpu/Material.h"
#include "core/render/Renderer.h"
#include "core/types/glm_conversions.h"

namespace EngineCore
{
	DebugDrawer::~DebugDrawer() = default;

	DebugDrawer::DebugDrawer(EngineDevice& device, DescriptorSet& defaultSet, const RenderingFormats& formats, VkSampleCountFlagBits samples)
		: device{ device }, defaultSet{ defaultSet }
	{
		// setup box mesh
		MeshBuilder builder{};
		builder.makeCubeMeshWireframe();
		boxMesh = std::make_unique<Nodes::MeshNode>();
		boxMesh->setDevice(device);
		boxMesh->build(builder);

		// setup debug primitive material
		auto shader = ShaderFilePaths(makePath("shaders/debug_primitive.vert.spv"), makePath("shaders/debug_primitive.frag.spv"));
		auto layouts = std::vector<VkDescriptorSetLayout>{ defaultSet.getLayout() };
		auto matInfo = MaterialCreateInfo(shader, layouts, samples, formats, sizeof(ShaderPushConstants::DebugPrimitivePushConstants));
		//matInfo.shadingProperties.enableDepth = false;
		matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		matInfo.shadingProperties.polygonMode = VK_POLYGON_MODE_LINE;
		matInfo.shadingProperties.lineWidth = 4.f;
		boxMesh->setMaterial(matInfo);
	}

	void DebugDrawer::addDebugBox(Vec dimensions, Vec location, Vec color, float opacity)
	{
		DDPushConstant pc;
		Transform transform(location, Vec::zero(), dimensions);
		pc.transform = cglm::transformToGLMmat4(transform);
		pc.color = { color.x, color.y, color.z, opacity };
		if (!hasPushConstantBox(pc))
			boxPushConstants.push_back(pc);
	}

	void DebugDrawer::removeDebugBoxes()
	{
		boxPushConstants.clear();
	}

	void DebugDrawer::render(VkCommandBuffer cmdBuffer, Renderer& renderer)
	{
		// called after the base renderpass has been initiated'

		auto material = boxMesh->getMaterial();
		material->bindToCommandBuffer(cmdBuffer);
		boxMesh->bind(cmdBuffer);

		auto sets = std::vector<VkDescriptorSet>{ defaultSet.getDescriptorSet(renderer.getFrameIndex()) };

		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipelineLayout(), 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

		for (DDPushConstant& box : boxPushConstants)
		{
			material->writePushConstants(cmdBuffer, box);
			boxMesh->draw(cmdBuffer);
		}
	}

	bool DebugDrawer::hasPushConstantBox(const DDPushConstant& compareBox) const
	{
		for (const DDPushConstant& box : boxPushConstants)
		{
			if (box.color == compareBox.color && box.transform == compareBox.transform)
				return true;
		}
		return false;
	}

}