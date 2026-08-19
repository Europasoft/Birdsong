// Copyright 2026 Simon Liimatainen. All rights reserved.
#include "core/draw/ViewportDrawer.h"
#include "core/draw/FrameContext.h"

#include "core/gpu/descriptors/DescriptorSetLayout.h"
#include "core/gpu/descriptors/DescriptorPool.h"
#include "core/gpu/Descriptors.h"
#include "core/gpu/Material.h"
#include "core/render/Renderer.h"

namespace EngineCore
{
	ViewportDrawer::ViewportDrawer(EngineDevice& device, const DrawContext& d)
		: DrawBase(device, d)
	{
		// draws the viewport, sampling from the framebuffer attachment of the previous pass
		// final post-processing can be applied in this pass, as it renders directly into the swapchain

		// descriptor set 1
		// attachments use the same image count as the swapchain, so that number is used instead of MAX_FRAMES_IN_FLIGHT
		auto inputImageViews = d.renderer->getPostFxPassInputImageViews();
		attachmentSet = std::make_unique<DescriptorSet>(device, (uint32_t)inputImageViews.size());
		ImageArrayDescriptor inputImages{}; // rendered attachment image(s) from the previous renderpass
		inputImages.addImage(inputImageViews);
		attachmentSet->addImageArray(inputImages);
		attachmentSet->finalize();

		// descriptor set 2 (sampler)
		samplerSet = std::make_unique<DescriptorSet>(device);
		samplerSet->addSampler(createSampler()); // the sampler is not connected to any specific image, but the shader needs one
		samplerSet->finalize();

		// bind the first scene-global set, and the 2 specialized ones
		auto layouts = std::vector<VkDescriptorSetLayout>{ d.world->getScene().getDescriptorSetLayouts()[0], attachmentSet->getLayout(), samplerSet->getLayout() };

		// setup material for the viewport (no mesh, a fullscreen rectangle is created in the vertex shader)
		ShaderFilePaths shaderPaths(makePath("shaders/compiled/viewport.vert.spv"), makePath("shaders/compiled/viewport.frag.spv"));
		MaterialCreateInfo info(shaderPaths, layouts, VK_SAMPLE_COUNT_1_BIT, d.postFxPassFormats, sizeof(ShaderPushConstants::ViewportPushConstants), EMatSet::NO);
		info.shadingProperties.useVertexInput = false;
		info.shadingProperties.enableDepth = false;
		info.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		viewportMaterial = std::make_unique<Material>(info, device);
		viewportMaterial->finalize();
	}

	void ViewportDrawer::render(const FrameContext& f)
	{
		const auto& frameIndex = d.renderer->getFrameIndex();
		const auto& imageIndex = d.renderer->getSwapImageIndex();

		d.renderer->beginRenderingPostFx(f.commandBuffer); // POST-FX PASS START

		bindDescriptorSets(f.commandBuffer, viewportMaterial.get()->getPipelineLayout(), frameIndex, imageIndex);

		viewportMaterial->bindToCommandBuffer(f.commandBuffer);

		// viewport does not necessarily cover the entire window
		ShaderPushConstants::ViewportPushConstants push{};
		push.positionAndSize.x = viewportPosition.x;
		push.positionAndSize.y = viewportPosition.y;
		push.positionAndSize.z = viewportSize.x;
		push.positionAndSize.w = viewportSize.y;
		assert(push.positionAndSize.z * push.positionAndSize.w > 0);
		viewportMaterial->writePushConstants(f.commandBuffer, push);

		// draw viewport
		vkCmdDraw(f.commandBuffer, 6, 1, 0, 0);

		d.renderer->endRendering(f.commandBuffer); // POST-FX PASS END
	}

	void ViewportDrawer::setPositionAndSize(Vec2 position, Vec2 size)
	{
		viewportPosition = position;
		viewportSize = size;
	}

	void ViewportDrawer::bindDescriptorSets(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, uint32_t frameIndex, uint32_t swapImageIndex)
	{
		// note that sets 0-1 use frame index, but set 2 uses swapchain image index
		std::array<VkDescriptorSet, 3> vkSets = { d.world->getScene().getDescriptorSets(frameIndex)[0], attachmentSet->getDescriptorSet(swapImageIndex), samplerSet->getDescriptorSet(frameIndex) };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 3, vkSets.data(), 0, nullptr);
	}

	VkSampler ViewportDrawer::createSampler()
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

	ViewportDrawer::~ViewportDrawer()
	{
		vkDestroySampler(device.device(), attachmentSampler, nullptr);
	}

}