#include "Core/Draw/SkyDrawer.h"
#include "Core/Primitive.h"
#include "Core/GPU/Device.h"
#include "Core/Types/CommonTypes.h"
#include "Core/GPU/Material.h"

namespace EngineCore
{
	SkyDrawer::SkyDrawer(const std::vector<VkDescriptorSetLayout>& setLayouts,
									EngineDevice& device, VkSampleCountFlagBits samples, VkRenderPass renderpass)
	{
		// TODO: hardcoded paths
		const std::string meshPath = makePath("Meshes/skysphere.obj");
		ShaderFilePaths skyShaders(makePath("Shaders/sky.vert.spv"), makePath("Shaders/sky.frag.spv"));

		// prepare sky mesh
		Primitive::MeshBuilder builder{};
		builder.loadFromFile(meshPath);
		skyMesh = std::make_unique<Primitive>(device, builder);
		skyMesh->getTransform().scale = 100000.f;

		// create unique material for sky, set to render backfaces, since it will be viewed from inside
		MaterialCreateInfo matInfo(skyShaders, setLayouts, samples, renderpass, sizeof(ShaderPushConstants::MeshPushConstants));
		matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		skyMesh->setMaterial(matInfo);
	}

	void SkyDrawer::renderSky(VkCommandBuffer commandBuffer, VkDescriptorSet sceneGlobalDescriptorSet, 
									const glm::vec3& observerPosition)
	{
		// aliases for convenience
		auto& sky = *skyMesh.get(); 
		auto skyMat = sky.getMaterial();

		skyMat->bindToCommandBuffer(commandBuffer); // bind sky shader pipeline

		// bind scene global descriptor set
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyMat->getPipelineLayout(),
									0, 1, &sceneGlobalDescriptorSet, 0, nullptr);

		// sky mesh position should be centered at the observer (camera) at all times
		Transform otf{}; // zero init transform, only translation is relevant
		otf.translation = observerPosition;
		ShaderPushConstants::MeshPushConstants push{};
		push.transform = otf.mat4();
		skyMat->writePushConstants(commandBuffer, push);

		// record draw command for sky mesh
		sky.bind(commandBuffer);
		sky.draw(commandBuffer);
	}

}