#pragma once
#include "Core/GPU/Swapchain.h"
#include "Core/Render/Attachment.h"
#include "Core/GPU/Material.h" // for RenderingFormats

#include <memory>
#include <vector>
#include <cassert>
#include <functional>

namespace EngineCore
{
	class EngineDevice;
	class EngineWindow;
	struct EngineRenderSettings;

	class Renderer
	{
	public:
		Renderer(EngineWindow& window, EngineDevice& device, EngineRenderSettings& renderSettings);
		~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		// rendering format info for pipeline creation (VK_KHR_dynamic_rendering)
		const RenderingFormats& getBasePassFormats() const { return basePassFormats; }
		const RenderingFormats& getFxPassFormats() const { return fxPassFormats; }

		bool getIsFrameInProgress() const { return isFrameStarted; }

		VkCommandBuffer getCurrentCommandBuffer() const 
		{ 
			assert(isFrameStarted && "getCurrentCommandBuffer failed, no frame in progress");
			return commandBuffers[currentFrameIndex];
		}

		int getFrameIndex() const
		{
			assert(isFrameStarted && "getFrameIndex failed, no frame in progress");
			return currentFrameIndex;
		}

		uint32_t getSwapImageIndex() const { return currentImageIndex; }

		float getSwapchainAspectRatio() const;
		VkExtent2D getSwapchainExtent() const { return swapchain->getExtent(); }

		// returns a command buffer to record commands into
		VkCommandBuffer beginFrame();
		// submit command buffer to finalize the frame
		void endFrame();

		// dynamic rendering (VK_KHR_dynamic_rendering)
		void beginRenderingBase(VkCommandBuffer cmdBuffer);
		void beginRenderingFx(VkCommandBuffer cmdBuffer);
		void endRendering(VkCommandBuffer cmdBuffer);

		const std::vector<VkImageView>& getFxPassInputImageViews() const { return fxPassInputImageViews; }
		const std::vector<VkImageView>& getFxPassInputDepthImageViews() const { return fxPassInputDepthImageViews; }

		std::function<void(void)> swapchainCreatedCallback;

	private:
		void createCommandBuffers();
		void freeCommandBuffers();

		// constructs attachments and swapchain
		void create();
		
		void createSwapchain();
		void createAttachments();
		const Attachment& addAttachment(const AttachmentProperties& p, bool inputAttachment, bool sampled) 
		{ 
			attachments.push_back(std::make_unique<Attachment>(device, p, inputAttachment, sampled));
			return *attachments.back(); 
		}

		// image layout transition helper
		void transitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, 
									VkImageLayout oldLayout, VkImageLayout newLayout,
									VkAccessFlags srcAccess, VkAccessFlags dstAccess,
									VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
									VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

		std::vector<std::unique_ptr<Attachment>> attachments;
		std::vector<VkImageView> fxPassInputImageViews; // view(s) to the color attachment image rendered by the first renderpass
		std::vector<VkImageView> fxPassInputDepthImageViews;
		
		// rendering format info for VK_KHR_dynamic_rendering
		RenderingFormats basePassFormats;
		RenderingFormats fxPassFormats;

		// attachment pointers for dynamic rendering (non-owning, attachments vector owns them)
		const Attachment* colorAttachment = nullptr;
		const Attachment* colorResolveAttachment = nullptr;
		const Attachment* depthAttachment = nullptr;
		const Attachment* depthResolveAttachment = nullptr;

		EngineWindow& window;
		EngineDevice& device;
		EngineRenderSettings& renderSettings;
		std::unique_ptr<EngineSwapChain> swapchain;
		std::vector<VkCommandBuffer> commandBuffers;
		// index of the current swapchain image
		uint32_t currentImageIndex;
		// index of the current frame, 0 - MAX_FRAMES_IN_FLIGHT
		int currentFrameIndex{ 0 };
		bool isFrameStarted{ false };
	};

}