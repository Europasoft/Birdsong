#pragma once
#include "Core/GPU/Swapchain.h"
#include "Core/Render/Renderpass.h"
#include "Core/Render/Attachment.h"

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

		Renderpass& getBaseRenderpass() { return *baseRenderpass.get(); }
		Renderpass& getFxRenderpass() { return *fxRenderpass.get(); }

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

		void beginRenderpassBase(VkCommandBuffer cmdBuffer);
		void beginRenderpassFx(VkCommandBuffer cmdBuffer);
		
		void endRenderpass()
		{
			assert(isFrameStarted && "failed to end renderpass, no frame in progress");
			vkCmdEndRenderPass(getCurrentCommandBuffer());
		}

		const std::vector<VkImageView>& getFxPassInputImageViews() const { return fxPassInputImageViews; }
		const std::vector<VkImageView>& getFxPassInputDepthImageViews() const { return fxPassInputDepthImageViews; }

		std::function<void(void)> swapchainCreatedCallback;

	private:
		void createCommandBuffers();
		void freeCommandBuffers();

		// constructs a new set of renderpasses and a swapchain
		void create();
		
		void createSwapchain();
		void createRenderpasses();
		const Attachment& addAttachment(const AttachmentProperties& p, bool inputAttachment, bool sampled) 
		{ 
			attachments.push_back(std::make_unique<Attachment>(device, p, inputAttachment, sampled));
			return *attachments.back(); 
		}

		std::unique_ptr<Renderpass> baseRenderpass;
		std::unique_ptr<Renderpass> fxRenderpass;
		std::vector<std::unique_ptr<Attachment>> attachments;
		std::vector<VkImageView> fxPassInputImageViews; // view(s) to the color attachment image rendered by the first renderpass
		std::vector<VkImageView> fxPassInputDepthImageViews;
		
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