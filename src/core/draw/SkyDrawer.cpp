#include "core/draw/SkyDrawer.h"
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

	SkyDrawer::SkyDrawer(EngineDevice& device, WorldSystem::World& world, const RenderingFormats& formats, VkSampleCountFlagBits samples)
		: world(world)
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
		
		auto layouts = world.getScene().getDescriptorSetLayouts();
		MaterialCreateInfo matInfo(skyShaders, layouts, samples, formats, sizeof(ShaderPushConstants::EngineMeshPushConstants), EMatSet::NO);
		matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		enodeSky->mesh->setMaterial(matInfo);
		enodeSky->mesh->getMaterial()->finalize();
	}

	void SkyDrawer::renderSky(uint32_t frameIndex, VkCommandBuffer commandBuffer, VkDescriptorSet sceneGlobalDescriptorSet, Vec observerPosition)
	{
		// aliases for convenience
		auto skyMat = enodeSky->mesh->getMaterial();

		skyMat->bindToCommandBuffer(commandBuffer); // bind sky shader pipeline

		// bind descriptor sets
		auto sets = world.getScene().getDescriptorSets(frameIndex);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyMat->getPipelineLayout(),
									0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

		// sky mesh position should be centered at the observer (camera) at all times
		Transform otf{}; // zero init transform, only translation is relevant
		otf.translation = observerPosition;
		otf.scale = { skyMeshScale, skyMeshScale, skyMeshScale };
		ShaderPushConstants::EngineMeshPushConstants push{};
		push.transform = cglm::transformToGLMmat4(otf);
		skyMat->writePushConstants(commandBuffer, push);

		// record draw command for sky mesh
		enodeSky->mesh->bind(commandBuffer);
		enodeSky->mesh->draw(commandBuffer);
	}

}