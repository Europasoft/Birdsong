#pragma once
#include "Core/Types/vk.h"
#include "Core/Render/Attachment.h"

#include <vector>
#include <memory>

namespace EngineCore 
{
	class Renderer;
	class EngineDevice;

	// acquires shared ownership of the Attachment objects
	class Renderpass 
	{
	public:
		Renderpass(EngineDevice& device, const std::vector<AttachmentUse>& attachmentUses, 
					VkExtent2D framebufferExtent, uint32_t framebufferCount);
		~Renderpass();
		Renderpass(Renderpass&&) = default;

		// uses the supplied command buffer to begin the renderpass
		void begin(VkCommandBuffer cmdBuffer, uint32_t framebufferIndex);

		VkRenderPass getRenderpass() const { return renderpass; }

	private:
		VkRenderPass renderpass;
		std::vector<VkFramebuffer> framebuffers; // one for each swapchain image
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		class EngineDevice& device;

		std::vector<AttachmentUse> attachments;
		std::vector<VkAttachmentDescription2> attachmentDescriptions;

		VkExtent2D framebufferExtent;
		uint32_t framebufferCount;
		
		bool areAttachmentsCompatible() const;
		std::vector<VkAttachmentDescription2> getAttachmentDescriptions() const;
		void createRenderpass();
		void createAttachmentReferences(std::vector<VkAttachmentReference2>& color, std::vector<VkAttachmentReference2>& resolve, 
										VkAttachmentReference2& depthStencil, VkAttachmentReference2& depthStencilResolve, 
										bool& hasDepth, bool& hasDepthResolve);
										
		void createFramebuffers();

	};

}
