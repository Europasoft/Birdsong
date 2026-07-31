#include "core/draw/FxDrawer.h"
#include "core/gpu/descriptors/DescriptorSetLayout.h"
#include "core/gpu/descriptors/DescriptorPool.h"
#include "core/engine/MeshData.h"
#include "core/types/CommonTypes.h"
#include "core/gpu/Descriptors.h"
#include "core/gpu/Material.h"
#include "core/render/Renderer.h"
#include "core/nodes/EngineNodeData.h"
#include "core/nodes/EMesh.h"
#include "core/types/glm_conversions.h"

namespace EngineCore
{
	FxDrawer::FxDrawer(EngineDevice& device, DescriptorSet& defaultSet, const RenderingFormats& formats,
						const std::vector<VkImageView>& inputImageViews, 
						const std::vector<VkImageView>& inputDepthImageViews)
		: device{ device }, defaultSet{ defaultSet }
	{
		// draws the whole frame again, sampling from the framebuffer attachment of the previous pass
		// after the fullscreen draw, translucent meshes are drawn
		// this allows translucent shaders to sample any pixel from the whole frame

		// descriptor set 1
		// attachments use the same image count as the swapchain, so that number is used instead of MAX_FRAMES_IN_FLIGHT
		attachmentSet = std::make_unique<DescriptorSet>(device, (uint32_t)inputImageViews.size());
		ImageArrayDescriptor inputImages{}; // rendered attachment image(s) from the previous renderpass
		inputImages.addImage(inputImageViews);
		attachmentSet->addImageArray(inputImages);
		attachmentSet->finalize();

		// descriptor set 2
		uboSet = std::make_unique<DescriptorSet>(device);
		UBO_Struct ubo{};
		ubo.add(uelem::vec2); // viewport extent value to be used in shader
		uboSet->addUBO(ubo, device);
		uboSet->addSampler(createSampler()); // the sampler is not connected to any specific image, but the shader needs one
		uboSet->finalize();

		// bind the scene-global set, and the 2 specialized ones
		auto layouts = std::vector<VkDescriptorSetLayout>{ defaultSet.getLayout(), attachmentSet->getLayout(), uboSet->getLayout() };

		// setup material for the fullscreen shaders (no mesh, a fullscreen triangle is created in the vertex shader)
		ShaderFilePaths fullscreenShader(makePath("shaders/compiled/fullscreen.vert.spv"), makePath("shaders/compiled/fullscreen.frag.spv"));
		MaterialCreateInfo fullscreenInfo(fullscreenShader, layouts, VK_SAMPLE_COUNT_1_BIT, formats, 0, EMatSet::NO);
		fullscreenInfo.shadingProperties.useVertexInput = false;
		fullscreenInfo.shadingProperties.enableDepth = false;
		fullscreenInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		fullscreenMaterial = std::make_unique<Material>(fullscreenInfo, device);
		fullscreenMaterial->finalize();

		// translucent teapot mesh with refraction
		// setup mesh and material (this mesh is engine-only, the node is not managed by the game and does not participate in any sector)
		enode = std::make_unique<WorldSystem::EngineNodeData>(nullptr, device);
		enode->mesh = std::make_unique<WorldSystem::Mesh>(device);
		enode->mesh->build("meshes/teapot.obj"); // load mesh from file

		ShaderFilePaths shader(makePath("shaders/compiled/fx_test.vert.spv"), makePath("shaders/compiled/fx_test.frag.spv"));
		enode->mesh->setMaterial(MaterialCreateInfo(shader, layouts, VK_SAMPLE_COUNT_1_BIT, formats, sizeof(ShaderPushConstants::EngineMeshPushConstants), EMatSet::NO));
		enode->mesh->getMaterial()->finalize();
		enode->engineTransform = Transform(Vec(-80.f, 0.f, 0.f), Vec(), Vec(5.f));
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
		auto material = enode->mesh->getMaterial();
		bindDescriptorSets(cmdBuffer, material->getPipelineLayout(), frameIndex, imageIndex);
		material->bindToCommandBuffer(cmdBuffer);
		
		ShaderPushConstants::EngineMeshPushConstants push{};
		push.transform = cglm::transformToGLMmat4(enode->engineTransform);
		material->writePushConstants(cmdBuffer, push);

		enode->mesh->bind(cmdBuffer);
		enode->mesh->draw(cmdBuffer);

		renderer.endRendering(cmdBuffer); // FX PASS END
	}

	void FxDrawer::bindDescriptorSets(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, uint32_t frameIndex, uint32_t swapImageIndex)
	{
		// note that sets 0-1 use frame index, but set 2 uses swapchain image index
		std::array<VkDescriptorSet, 3> vkSets = { defaultSet.getDescriptorSet(frameIndex), attachmentSet->getDescriptorSet(swapImageIndex), uboSet->getDescriptorSet(frameIndex) };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 3, vkSets.data(), 0, nullptr);
	}

	VkSampler FxDrawer::createSampler()
	{
		VkSamplerCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		// exact 1:1 sampling for full-screen pass
		info.magFilter = VK_FILTER_NEAREST;
		info.minFilter = VK_FILTER_NEAREST;
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		// prevent any edge bleeding outside [0, 1]
		info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		// no mipmaps
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		info.minLod = 0.f;
		info.maxLod = 0.f;

		Image::createSampler(attachmentSampler, device, info);
		return attachmentSampler;
	}

	FxDrawer::~FxDrawer()
	{
		vkDestroySampler(device.device(), attachmentSampler, nullptr);
	}

}