#include "core/draw/DebugDrawer.h"
#include "core/draw/FrameContext.h"

#include "core/engine/MeshData.h"
#include "core/world/World.h"
#include "core/gpu/Descriptors.h"
#include "core/gpu/Material.h"
#include "core/render/Renderer.h"
#include "core/nodes/EngineNodeData.h"
#include "core/nodes/EMesh.h"
#include "core/types/glm_conversions.h"

namespace EngineCore
{
	DebugDrawer::DebugDrawer(EngineDevice& device, const DrawContext& d)
		: DrawBase(device, d)
	{
		// setup box mesh
		enodeBox = std::make_unique<WorldSystem::EngineNodeData>(nullptr, device);
		enodeBox->mesh = std::make_unique<WorldSystem::Mesh>(device);
		MeshBuilder builder{};
		builder.makeCubeMeshWireframe();
		enodeBox->mesh->build(builder);

		// setup debug primitive material
		auto shader = ShaderFilePaths(makePath("shaders/compiled/debug_primitive.vert.spv"), makePath("shaders/compiled/debug_primitive.frag.spv"));
		auto layouts = d.world->getScene().getDescriptorSetLayouts();
		auto matInfo = MaterialCreateInfo(shader, layouts, d.samples, d.basePassFormats, sizeof(ShaderPushConstants::DebugPrimitivePushConstants));
		//matInfo.shadingProperties.enableDepth = false;
		matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		matInfo.shadingProperties.polygonMode = VK_POLYGON_MODE_LINE;
		matInfo.shadingProperties.lineWidth = 4.f;
		enodeBox->mesh->setMaterial(matInfo);
		enodeBox->mesh->getMaterial()->finalize();
	}
	
	DebugDrawer::~DebugDrawer() = default;

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

    void DebugDrawer::render(const FrameContext& f)
	{
		
		// called after the base renderpass has been initiated'
		auto material = enodeBox->mesh->getMaterial();
		material->bindToCommandBuffer(f.commandBuffer);
		enodeBox->mesh->bind(f.commandBuffer);

		auto sets = f.world->getScene().getDescriptorSets(f.bufferIndex);
		vkCmdBindDescriptorSets(f.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipelineLayout(), 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

		for (DDPushConstant& box : boxPushConstants)
		{
			material->writePushConstants(f.commandBuffer, box);
			enodeBox->mesh->draw(f.commandBuffer);
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