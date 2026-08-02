#pragma once

#include "core/engine/Window.h"

// std lib headers
#include <string>
#include <vector>
#include <memory>
#include <mutex>

class EngineApplication; 

namespace EngineCore 
{
	class Fence; // for AsyncCommandDispatcher
	class AsyncCommandDispatcher;

	struct SwapChainSupportDetails 
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices 
	{
		uint32_t graphicsFamily;
		uint32_t presentFamily;
		bool graphicsFamilyHasValue = false;
		bool presentFamilyHasValue = false;
		bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
	};

	class EngineDevice 
	{
	public:
#ifdef NDEBUG
		static constexpr bool enableValidationLayers = false;
#else
		static constexpr bool enableValidationLayers = true;
#endif
		EngineDevice(EngineWindow& window);
		~EngineDevice();

		EngineDevice(const EngineDevice&) = delete;
		EngineDevice& operator=(const EngineDevice&) = delete;
		EngineDevice& operator=(EngineDevice&&) = delete;
		EngineDevice(EngineDevice&&) = delete;

		VkCommandPool getCommandPool() { return commandPool; }
		VkDevice device() const { return device_; }
		VkSurfaceKHR surface() { return surface_; }
		VkQueue graphicsQueue() { return graphicsQueue_; }
		VkQueue presentQueue() { return presentQueue_; }
		VkInstance getVulkanInstance() { return instance; } // for imgui

		SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
		VkFormat findSupportedFormat(
			const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		// remember to compare against VK_NULL_HANDLE
		VkPhysicalDevice& getPhysicalDevice() { return physicalDevice; }
		// checks device properties to get the max samples supported for both color and depth
		VkSampleCountFlagBits getMaxSampleCount();
		AsyncCommandDispatcher& getAsyncCommandDispatcher() const;
		void submitAsyncCommandDispatcherBuffers() const;

		// Buffer Helper Functions
		void createBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory);
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void copyBufferToImage(
			VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);
		// creates a vulkan image object
		void createImageWithInfo(
			const VkImageCreateInfo& imageInfo,
			VkMemoryPropertyFlags properties,
			VkImage& image,
			VkDeviceMemory& imageMemory);
		// imports and initializes an image texture from disk
		//void importImageFromFile(const char* path);
		// takes a VkImage and transitions its layout
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		VkPhysicalDeviceProperties properties;

	private:
		// createInstance instantiates the vulkan library
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createCommandPool();

		// helper functions
		bool isDeviceSuitable(VkPhysicalDevice device);
		std::vector<const char*> getRequiredExtensions();
		bool checkValidationLayerSupport();
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void hasGflwRequiredInstanceExtensions();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		// the vulkan library instance
		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		EngineWindow& window;
		VkCommandPool commandPool;
		// logical device
		VkDevice device_;
		VkSurfaceKHR surface_;
		VkQueue graphicsQueue_;
		VkQueue presentQueue_;

		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		std::unique_ptr<AsyncCommandDispatcher> asyncCommandDispatcher;
	};

	class AsyncCommandBuffer
	{
	public:
		AsyncCommandBuffer(EngineDevice& device, VkCommandPool pool);
		~AsyncCommandBuffer();

		VkCommandBuffer get() const { return buf; }
		void markForSubmit() { readyToSubmit = true; }
		bool finished() const;

	private:
		EngineDevice& device;
		VkCommandPool pool;
		VkCommandBuffer buf = VK_NULL_HANDLE;
		std::unique_ptr<Fence> fence;

		friend class AsyncCommandDispatcher;
		bool readyToSubmit = false;
	};

	class AsyncCommandDispatcher
	{
	public:
		AsyncCommandDispatcher(EngineDevice& device);
		~AsyncCommandDispatcher();

	public:
		std::shared_ptr<AsyncCommandBuffer> startNewCommandBuffer(std::unique_lock<std::mutex>& lock);
		

	private:
		EngineDevice& device;
		VkQueue asyncQueue = VK_NULL_HANDLE;
		VkCommandPool asyncCommandPool = VK_NULL_HANDLE;
		std::mutex m;

		std::vector<std::shared_ptr<AsyncCommandBuffer>> cmdBuffers;

		friend EngineDevice;
		void init(uint32_t familyIndex, uint32_t queueIndex);
		void submitAll();
		void destroy();
	};

}
