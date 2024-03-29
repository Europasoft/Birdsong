#pragma once

#include "Core/GPU/Device.h"

// warning-ignore hack only works in header
#pragma warning(push, 0) 
#include <vulkan/vulkan.h> // include vulkan
#pragma warning(pop)

#include <string>
#include <vector>
#include <memory>
#include <cassert>

namespace EngineCore {

	class EngineSwapChain {
	public:
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		EngineSwapChain(EngineDevice& deviceRef, VkExtent2D extent, VkSampleCountFlagBits samples);
		EngineSwapChain(EngineDevice& deviceRef, VkExtent2D extent, VkSampleCountFlagBits samples,
						std::shared_ptr<EngineSwapChain> previous);
		~EngineSwapChain();
		void init(VkSampleCountFlagBits samples);

		EngineSwapChain(const EngineSwapChain&) = delete;
		EngineSwapChain& operator=(const EngineSwapChain&) = delete;

		VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
		VkRenderPass getRenderPass() { return renderPass; }
		VkImageView getImageView(int index) { return swapChainImageViews[index]; }
		size_t imageCount() { return swapChainImages.size(); }
		VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
		VkExtent2D getSwapChainExtent() { return swapChainExtent; }
		uint32_t width() { return swapChainExtent.width; }
		uint32_t height() { return swapChainExtent.height; }

		float getExtentAspectRatio() const {
			return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
		}
		VkFormat findDepthFormat();

		VkResult acquireNextImage(uint32_t* imageIndex);
		VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

		bool compareSwapFormats(const EngineSwapChain& swapchain) const 
		{
			return (swapchain.swapChainDepthFormat == swapChainDepthFormat)
					&& (swapchain.swapChainImageFormat == swapChainImageFormat); }

	private:
		void createSwapChain();
		void createImageViews();
		void createDepthResources(VkSampleCountFlagBits samples);
		void createColorResources(VkSampleCountFlagBits samples);
		void createRenderPass(VkSampleCountFlagBits samples);
		void createFramebuffers();
		void createSyncObjects();

		// Helper functions
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		VkFormat swapChainImageFormat;
		VkFormat swapChainDepthFormat;
		VkExtent2D swapChainExtent;

		std::vector<VkFramebuffer> swapChainFramebuffers;
		VkRenderPass renderPass;

		// framebuffer attachments
		std::vector<VkImage> depthImages;
		std::vector<VkDeviceMemory> depthImageMemorys;
		std::vector<VkImageView> depthImageViews;
		std::vector<VkImage> multisampleImages;
		std::vector<VkDeviceMemory> multisampleImageMemorys;
		std::vector<VkImageView> multisampleImageViews;

		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;

		EngineDevice& device;
		VkExtent2D windowExtent;

		VkSwapchainKHR swapChain;
		std::shared_ptr<EngineSwapChain> oldSwapChain;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		size_t currentFrame = 0;
	};

}  // namespace