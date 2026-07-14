#include "Core/Render/Renderpass.h"

#include "Core/GPU/Device.h"
#include "Core/Render/Renderer.h"
#include "Core/Render/Attachment.h"

#include <array>
#include <stdexcept>
#include <cassert>

namespace EngineCore 
{
	Renderpass::Renderpass(EngineDevice& device, const std::vector<AttachmentUse>& attachmentUses, 
							VkExtent2D framebufferExtent, uint32_t framebufferCount)
		: device{ device }, attachments{ attachmentUses }, framebufferExtent{ framebufferExtent }, framebufferCount{ framebufferCount }
	{
		assert(areAttachmentsCompatible() && "failed to create renderpass, incompatible attachment");
		createRenderpass();
		createFramebuffers();
	}

	Renderpass::~Renderpass() 
	{
		for (auto f : framebuffers) { vkDestroyFramebuffer(device.device(), f, nullptr); }
		vkDestroyRenderPass(device.device(), renderpass, nullptr);
	}

	bool Renderpass::areAttachmentsCompatible() const
	{
		//for (const auto& a : attachments) { if (!attachments[0]->isCompatible(*a.get())) { return false; } }
		return true;
	}

	void Renderpass::createRenderpass()
	{
		// descriptions and references for the attachments
		std::vector<VkAttachmentReference2> colorRefs, resolveRefs, depthResolveRefs;
		VkAttachmentReference2 depthStencilRef, depthStencilResolveRef;
		bool hasDepth, hasDepthResolve;
		createAttachmentReferences(colorRefs, resolveRefs, depthStencilRef, depthStencilResolveRef, hasDepth, hasDepthResolve);

		size_t numColor = colorRefs.size();
		size_t numResolve = resolveRefs.size();
		assert((numColor > 0 || hasDepth) && "no color or depth-stencil attachments specified");
		assert((numResolve < 1 || numColor == numResolve) && "resolve and color attachment counts mismatch");
		//assert((hasDepth || !hasDepthResolve) && "added depth resolve attachment but no depth attachment");

		// create subpass
		VkSubpassDescription2 subpass = {};
		subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = (uint32_t)numColor;
		subpass.pColorAttachments = numColor ? colorRefs.data() : NULL;
		subpass.pResolveAttachments = numResolve ? resolveRefs.data() : NULL;
		subpass.pDepthStencilAttachment = hasDepth ? &depthStencilRef : NULL;
		VkSubpassDescriptionDepthStencilResolve depthResolve = {};
		depthResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
		if (hasDepthResolve) 
		{
			depthResolve.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT; // sample zero mode is guaranteed to be supported
			depthResolve.stencilResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
			depthResolve.pDepthStencilResolveAttachment = &depthStencilResolveRef;
			subpass.pNext = &depthResolve;
		}

		VkSubpassDependency2 dependency = {};
		dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstSubpass = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		// create renderpass
		auto descriptions = getAttachmentDescriptions();
		VkRenderPassCreateInfo2 renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(descriptions.size());
		renderPassInfo.pAttachments = descriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass2(device.device(), &renderPassInfo, nullptr, &renderpass) != VK_SUCCESS)
		{ throw std::runtime_error("failed to create renderpass"); }
	}

	std::vector<VkAttachmentDescription2> Renderpass::getAttachmentDescriptions() const 
	{
		std::vector<VkAttachmentDescription2> d;
		d.reserve(attachments.size());
		for (const AttachmentUse& a : attachments) { d.push_back(a.description); }
		return d;
	}

	void Renderpass::createAttachmentReferences(std::vector<VkAttachmentReference2>& color, std::vector<VkAttachmentReference2>& resolve,
												VkAttachmentReference2& depthStencil, VkAttachmentReference2& depthStencilResolve, 
												bool& hasDepth, bool& hasDepthResolve)
												
	{
		using AT = AttachmentType;
		hasDepth = false;
		hasDepthResolve = false;
		uint32_t i = 0; // index for VkRenderPassCreateInfo pAttachments array
		for (const AttachmentUse& a : attachments)
		{
			VkAttachmentReference2 ref{};
			ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
			ref.attachment = i++;
			ref.layout = Attachment::isColor(a.type) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
														VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			switch (a.type)
			{
			case AT::COLOR:
				color.push_back(ref);
				break;
			case AT::RESOLVE:
				resolve.push_back(ref);
				break;
			case AT::DEPTH: case AT::DEPTH_STENCIL: 
				assert(!hasDepth && "cannot add more than one depth stencil attachment");
				hasDepth = true;
				depthStencil = ref;
				break;
			case AT::DEPTH_STENCIL_RESOLVE:
				assert(!hasDepthResolve && "cannot add more than one depth stencil resolve attachment");
				hasDepthResolve = true;
				depthStencilResolve = ref;
			}
		}
	}

	void Renderpass::createFramebuffers()
	{
		framebuffers.resize(framebufferCount);
		for (size_t i = 0; i < framebufferCount; i++)
		{
			// for each attachment, get the corresponding view
			std::vector<VkImageView> views;
			views.reserve(attachments.size());
			for (auto& a : attachments) { views.push_back(a.imageViews[i]); }

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderpass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
			framebufferInfo.pAttachments = views.data();
			framebufferInfo.width = framebufferExtent.width;
			framebufferInfo.height = framebufferExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
			{ throw std::runtime_error("failed to create framebuffer"); }
		}
	}

	void Renderpass::begin(VkCommandBuffer cmdBuffer, uint32_t framebufferIndex)
	{
		assert(cmdBuffer != VK_NULL_HANDLE && "begin renderpass failed, no command buffer");
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderpass;
		renderPassInfo.framebuffer = framebuffers[framebufferIndex];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = framebufferExtent;

		//std::array<VkClearValue, 2> clearValues{};
		VkClearValue clearValueColor = { 0.1f, 0.12f, 0.2f, 1.0f };
		VkClearValue clearValueDepth = { 1.f, 0 };

		std::vector<VkClearValue> clearValues;
		clearValues.reserve(attachments.size());
		for (auto& a : attachments) 
		{
			if (Attachment::isColor(a.type)) { clearValues.push_back(clearValueColor); }
			else { clearValues.push_back(clearValueDepth); }
		}


		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(framebufferExtent.width);
		viewport.height = static_cast<float>(framebufferExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, framebufferExtent };
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
	}

}

