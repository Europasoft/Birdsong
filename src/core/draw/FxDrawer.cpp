#include "core/draw/FxDrawer.h"
#include "core/nodes/MeshNode.h"
#include "core/engine/MeshData.h"
#include "core/types/CommonTypes.h"
#include "core/gpu/Descriptors.h"
#include "core/gpu/Material.h"
#include "core/render/Renderer.h"
#include "core/types/glm_conversions.h"

namespace EngineCore
{
	FxDrawer::~FxDrawer() = default;

	FxDrawer::FxDrawer(EngineDevice& device, DescriptorSet& defaultSet, const RenderingFormats& formats,
						const std::vector<VkImageView>& inputImageViews, 
						const std::vector<VkImageView>& inputDepthImageViews)
		: device{ device }, defaultSet{ defaultSet }
	{
		// initialized as normal
		uboSet = std::make_unique<DescriptorSet>(device); 
		UBO_Struct ubo{};
		ubo.add(uelem::vec2); // viewport extent value to be used in shader
		uboSet->addUBO(ubo, device);
		uboSet->finalize();

		// attachments use the same image count as the swapchain, so that number is used instead of MAX_FRAMES_IN_FLIGHT
		attachmentSet = std::make_unique<DescriptorSet>(device, (uint32_t)inputImageViews.size()); // set 2
		ImageArrayDescriptor inputImages{}; // rendered attachment image(s) from the previous renderpass
		inputImages.addImage(inputImageViews);
		//inputImages.addImage(inputDepthImageViews);
		attachmentSet->addImageArray(inputImages);
		attachmentSet->finalize();

		auto layouts = std::vector<VkDescriptorSetLayout>{ defaultSet.getLayout(), uboSet->getLayout(), attachmentSet->getLayout() };

		// setup material for the fullscreen shaders (no mesh)
		ShaderFilePaths fullscreenShader(makePath("shaders/fullscreen.vert.spv"), makePath("shaders/fullscreen.frag.spv"));
		MaterialCreateInfo fullscreenInfo(fullscreenShader, layouts, VK_SAMPLE_COUNT_1_BIT, formats, 0);
		fullscreenInfo.shadingProperties.useVertexInput = false;
		fullscreenInfo.shadingProperties.enableDepth = false;
		fullscreenInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		fullscreenMaterial = std::make_unique<Material>(fullscreenInfo, device);

		// setup mesh and material
		MeshBuilder builder{};
		builder.loadFromFile(makePath("meshes/teapot.obj"));
		mesh = std::make_unique<Nodes::MeshNode>();
		mesh->setDevice(device);
		mesh->build(builder);
		ShaderFilePaths shader(makePath("shaders/fx_test.vert.spv"), makePath("shaders/fx_test.frag.spv"));
		mesh->setMaterial(MaterialCreateInfo(shader, layouts, VK_SAMPLE_COUNT_1_BIT, formats, sizeof(ShaderPushConstants::MeshPushConstants)));
		Transform tf(Vec(-80.f, 0.f, 0.f), Vec(), Vec(5.f));
		mesh->setTransform(tf);
	}

	void FxDrawer::render(VkCommandBuffer cmdBuffer, Renderer& renderer)
	{
		const auto& frameIndex = renderer.getFrameIndex();
		const auto& imageIndex = renderer.getSwapImageIndex();

		// update viewport extent descriptor value
		VkExtent2D extent = renderer.getSwapchainExtent();
		uboSet->writeUBOMember(0, extent, UBO_Layout::ElementAccessor{0, 0, 0}, frameIndex);

		renderer.beginRenderingFx(cmdBuffer); // FX PASS START

		// draw fullscreen
		bindDescriptorSets(cmdBuffer, fullscreenMaterial.get()->getPipelineLayout(), frameIndex, imageIndex);
		fullscreenMaterial->bindToCommandBuffer(cmdBuffer);
		vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

		// draw mesh
		bindDescriptorSets(cmdBuffer, mesh->getMaterial()->getPipelineLayout(), frameIndex, imageIndex);
		auto material = mesh->getMaterial();
		material->bindToCommandBuffer(cmdBuffer);
		
		ShaderPushConstants::MeshPushConstants push{};
		
		push.transform = cglm::transformToGLMmat4(mesh->getTransform());
		material->writePushConstants(cmdBuffer, push);

		mesh->bind(cmdBuffer);
		mesh->draw(cmdBuffer);

		renderer.endRendering(cmdBuffer); // FX PASS END
	}

	void FxDrawer::bindDescriptorSets(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, uint32_t frameIndex, uint32_t swapImageIndex)
	{
		// note that sets 0-1 use frame index, but set 2 uses swapchain image index
		std::array<VkDescriptorSet, 3> vkSets = { defaultSet.getDescriptorSet(frameIndex), uboSet->getDescriptorSet(frameIndex), attachmentSet->getDescriptorSet(swapImageIndex) };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 3, vkSets.data(), 0, nullptr);
	}



}