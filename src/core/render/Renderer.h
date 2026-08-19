#pragma once
#include "core/gpu/Swapchain.h"
#include "core/render/Attachment.h"
#include "core/gpu/Material.h" // for RenderingFormats
#include "core/draw/FrameContext.h"

#include <memory>
#include <vector>
#include <cassert>
#include <functional>

namespace EngineCore
{
	class EngineDevice;
	class EngineWindow;
	struct EngineRenderSettings;
	class EngineApplication;

	class Renderer
	{
	public:
		Renderer(EngineWindow& window, EngineDevice& device, EngineApplication& engine);
		~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		// rendering format info for pipeline creation (VK_KHR_dynamic_rendering)
		const RenderingFormats& getBasePassFormats() const { return basePassFormats; }
		const RenderingFormats& getFxPassFormats() const { return fxPassFormats; }
		const RenderingFormats& getPostFxPassFormats() const { return postFxPassFormats; }

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
		bool getNewViewportState();
		void getInitialViewportState();

		// returns a command buffer to record commands into
		VkCommandBuffer beginFrame();
		// submit command buffer to finalize the frame
		void endFrame();

		// dynamic rendering (VK_KHR_dynamic_rendering)
		void beginRenderingBase(VkCommandBuffer cmdBuffer);
		void beginRenderingFx(VkCommandBuffer cmdBuffer);
		void beginRenderingPostFx(VkCommandBuffer cmdBuffer);
		void endRendering(VkCommandBuffer cmdBuffer);

		const std::vector<VkImageView>& getFxPassInputImageViews() const { return fxPassInputImageViews; }
		const std::vector<VkImageView>& getFxPassInputDepthImageViews() const { return fxPassInputDepthImageViews; }
		const std::vector<VkImageView>& getPostFxPassInputImageViews() const { return postFxPassInputImageViews; }

		std::function<void(void)> swapchainCreatedCallback;

	private:
		void createCommandBuffers();
		void freeCommandBuffers();

		// reconstructs attachments and swapchain
		void recreate();
		
		void createSwapchain();
		void createAttachments();
		const Attachment& addAttachment(const AttachmentProperties& p, bool inputAttachment, bool sampled) 
		{ 
			attachments.push_back(std::make_unique<Attachment>(device, p, inputAttachment, sampled));
			return *attachments.back(); 
		}
		
		// image layout transition helper
		friend struct ImageLayoutChanger;
		static void transitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, 
									VkImageLayout oldLayout, VkImageLayout newLayout,
									VkAccessFlags srcAccess, VkAccessFlags dstAccess,
									VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
									VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

		std::vector<std::unique_ptr<Attachment>> attachments;
		std::vector<VkImageView> fxPassInputImageViews; // view(s) to the color attachment image rendered by the first renderpass
		std::vector<VkImageView> fxPassInputDepthImageViews;
		std::vector<VkImageView> postFxPassInputImageViews;
		
		// rendering format info for VK_KHR_dynamic_rendering
		RenderingFormats basePassFormats;
		RenderingFormats fxPassFormats;
		RenderingFormats postFxPassFormats;

		// attachment pointers for dynamic rendering (non-owning, attachments vector owns them)
		const Attachment* colorAttachment = nullptr;
		const Attachment* colorResolveAttachment = nullptr;
		const Attachment* depthAttachment = nullptr;
		const Attachment* depthResolveAttachment = nullptr;
		const Attachment* fxColorAttachment = nullptr;

		ViewportState viewportState{};
		VkExtent2D viewportExtent{ 0, 0 };

		EngineWindow& window;
		EngineDevice& device;
		const EngineRenderSettings& renderSettings;
		const EngineApplication& engine;
		std::unique_ptr<EngineSwapChain> swapchain;
		std::vector<VkCommandBuffer> commandBuffers;
		// index of the current swapchain image
		uint32_t currentImageIndex;
		// index of the current frame, 0 - MAX_FRAMES_IN_FLIGHT
		int currentFrameIndex{ 0 };
		bool isFrameStarted{ false };
	};

}