#include "core/draw/SkyDrawer.h"
#include "core/draw/FrameContext.h"

#include "core/engine/MeshData.h"
#include "core/gpu/Device.h"
#include "core/types/CommonTypes.h"
#include "core/gpu/Material.h"
#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/nodes/EngineNodeData.h"
#include "core/nodes/EMesh.h"
#include "core/types/glm_conversions.h"

namespace EngineCore
{
	SkyDrawer::~SkyDrawer() = default;

	SkyDrawer::SkyDrawer(EngineDevice& device, const DrawContext& d)
		: DrawBase(device, d)
	{
		// TODO: hardcoded paths
		ShaderFilePaths skyShaders(makePath("shaders/compiled/sky.vert.spv"), makePath("shaders/compiled/sky.frag.spv"));

		// prepare sky mesh
		enodeSky = std::make_unique<WorldSystem::EngineNodeData>(nullptr, device);
		enodeSky->mesh = std::make_unique<WorldSystem::Mesh>(device);
		enodeSky->mesh->build("meshes/skysphere.obj");
		Transform tf{};
		tf.scale = { 50.f };
		enodeSky->engineTransform = tf;

		// create unique material for sky, set to render backfaces, since it will be viewed from inside
		
		auto layouts = d.world->getScene().getDescriptorSetLayouts();
		MaterialCreateInfo matInfo(skyShaders, layouts, d.samples, d.basePassFormats, sizeof(ShaderPushConstants::EngineMeshPushConstants), EMatSet::NO);
		matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		enodeSky->mesh->setMaterial(matInfo);
		enodeSky->mesh->getMaterial()->finalize();
	}

	void SkyDrawer::render(const FrameContext& f)
	{
		// aliases for convenience
		auto skyMat = enodeSky->mesh->getMaterial();

		skyMat->bindToCommandBuffer(f.commandBuffer); // bind sky shader pipeline

		// bind descriptor sets
		auto sets = d.world->getScene().getDescriptorSets(f.bufferIndex);
		vkCmdBindDescriptorSets(f.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyMat->getPipelineLayout(),
									0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

		// sky mesh position should be centered at the observer (camera) at all times
		Transform otf{}; // zero init transform, only translation is relevant
		otf.translation = f.camera->transform.translation;
		otf.scale = { skyMeshScale, skyMeshScale, skyMeshScale };
		ShaderPushConstants::EngineMeshPushConstants push{};
		push.transform = cglm::transformToGLMmat4(otf);
		skyMat->writePushConstants(f.commandBuffer, push);

		// record draw command for sky mesh
		enodeSky->mesh->bind(f.commandBuffer);
		enodeSky->mesh->draw(f.commandBuffer);
	}

}