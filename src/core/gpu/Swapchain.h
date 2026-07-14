#pragma once
#include "core/types/vk.h"
#include "core/gpu/Device.h"
#include "core/render/Attachment.h"

#include <string>
#include <vector>
#include <memory>
#include <cassert>

namespace EngineCore 
{
	class EngineSwapChain 
	{
	public:
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		EngineSwapChain(EngineDevice& device, VkExtent2D windowExtent);
		EngineSwapChain(EngineDevice& device, VkExtent2D windowExtent, std::shared_ptr<EngineSwapChain> previous);
		~EngineSwapChain();

		EngineSwapChain(const EngineSwapChain&) = delete;
		EngineSwapChain& operator=(const EngineSwapChain&) = delete;

		// public getters
		//VkFramebuffer getFrameBuffer(int index) const { return swapChainFramebuffers[index]; }
		class Attachment& getSwapchainAttachment();
		std::vector<VkImage> getSwapChainImages() const;

		VkFormat getSwapChainImageFormat() const { return imageFormat; }
		VkFormat getImageFormat() const { return imageFormat; }
		VkFormat getDepthFormat() const { return depthFormat; }
		size_t getImageCount() const { return imageCount; }
		VkExtent2D getExtent() const { return extent; }
		uint32_t width() const { return getExtent().width; }
		uint32_t height() const { return getExtent().height; }
		float getExtentAspectRatio() const { return static_cast<float>(width()) / static_cast<float>(height()); }
		// returns image properties matching the current swapchain state
		AttachmentProperties getAttachmentProperties() const;
		bool compareSwapFormats(const EngineSwapChain& b) const { return (depthFormat == b.depthFormat) && (imageFormat == b.imageFormat); }

		VkResult acquireNextImage(uint32_t* imageIndex);
		VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

	private:
		void init(VkExtent2D windowExtent);
		void createSyncObjects();
		
		// helpers
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D windowExtent);
		VkFormat findDepthFormat(bool stencilRequired);

		VkFormat imageFormat;
		VkFormat depthFormat;
		VkExtent2D extent;
		uint32_t imageCount;

		std::unique_ptr<Attachment> swapchainAttachment; // swapchain images

		EngineDevice& device;

		VkSwapchainKHR swapchain;
		std::shared_ptr<EngineSwapChain> oldSwapchain;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		size_t currentFrame = 0;
	};

}