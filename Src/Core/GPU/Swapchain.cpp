#include "Core/GPU/Swapchain.h"

#include "Core/Render/Attachment.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace EngineCore 
{

	EngineSwapChain::EngineSwapChain(EngineDevice& device, VkExtent2D windowExtent)
		: device{ device }
	{
		init(windowExtent);
		createSyncObjects();
	}

	EngineSwapChain::EngineSwapChain(EngineDevice& device, VkExtent2D windowExtent, std::shared_ptr<EngineSwapChain> previous)
		: device{ device }, oldSwapchain{ previous }
	{
		init(windowExtent);
		createSyncObjects();
		oldSwapchain = nullptr;
	}

	EngineSwapChain::~EngineSwapChain() 
	{
		swapchainAttachment.reset(); // destroy swapchain images

		if (swapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device.device(), swapchain, nullptr);
			swapchain = VK_NULL_HANDLE;
		}

		// cleanup synchronization objects
		for (size_t i = 0; i < imageAvailableSemaphores.size(); i++) 
		{
			vkDestroySemaphore(device.device(), renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device.device(), inFlightFences[i], nullptr);
		}
	}

	void EngineSwapChain::init(VkExtent2D windowExtent)
	{
		SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		imageFormat = surfaceFormat.format;
		depthFormat = findDepthFormat(true);
		extent = chooseSwapExtent(swapChainSupport.capabilities, windowExtent);

		imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = device.surface();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = imageFormat;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;      // Optional
			createInfo.pQueueFamilyIndices = nullptr;  // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = !oldSwapchain ? VK_NULL_HANDLE : oldSwapchain->swapchain ;

		if (vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapchain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		// get swapchain image count (may be higher than VkSwapchainCreateInfoKHR::minImageCount)
		vkGetSwapchainImagesKHR(device.device(), swapchain, &imageCount, nullptr);
		std::vector<VkImage> swapImages(imageCount);
		vkGetSwapchainImagesKHR(device.device(), swapchain, &imageCount, swapImages.data()); // get images
		AttachmentProperties swapProperties = getAttachmentProperties();
		swapchainAttachment = std::make_unique<Attachment>(device, swapProperties, swapImages); // create swapchain attachments
	}

	Attachment& EngineSwapChain::getSwapchainAttachment() { return *swapchainAttachment.get(); }

	std::vector<VkImage> EngineSwapChain::getSwapChainImages() const 
	{ 
		return swapchainAttachment->getImages(); 
	}

	const AttachmentProperties& EngineSwapChain::getAttachmentProperties() const
	{
		AttachmentProperties props(AttachmentType::COLOR);
		props.extent = extent;
		props.format = imageFormat;
		props.imageCount = imageCount;
		props.samples = VK_SAMPLE_COUNT_1_BIT;
		return props;
	}

	VkResult EngineSwapChain::acquireNextImage(uint32_t* imageIndex)
	{
		vkWaitForFences(device.device(), 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		return vkAcquireNextImageKHR(device.device(), swapchain, std::numeric_limits<uint64_t>::max(), 
									imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, imageIndex);
	}

	VkResult EngineSwapChain::submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// use semaphores indexed by the acquired image
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = buffers;

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);
		if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		{ throw std::runtime_error("failed to submit draw command buffer!"); }

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = imageIndex;

		auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

		currentFrame = (currentFrame + 1) % imageCount;

		return result;
	}

	void EngineSwapChain::createSyncObjects() 
	{
		// use one set of sync objects per swapchain image to avoid semaphore reuse issues
		imageAvailableSemaphores.resize(imageCount);
		renderFinishedSemaphores.resize(imageCount);
		inFlightFences.resize(imageCount);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < imageCount; i++) 
		{
			if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) 
			{ throw std::runtime_error("failed to create synchronization objects for a frame"); }
		}
	}

	VkSurfaceFormatKHR EngineSwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
	{
		for (const auto& availableFormat : availableFormats) 
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
			{ return availableFormat; }
		}
		return availableFormats[0];
	}

	VkPresentModeKHR EngineSwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		/*for (const auto& availablePresentMode : availablePresentModes) 
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
			{
				std::cout << "Present mode: Mailbox" << std::endl;
				return availablePresentMode;
			}
		}*/

		// immediate mode does not sync to display refresh rate at all (max framerate)
		 for (const auto &availablePresentMode : availablePresentModes) 
		 {
		   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) 
		   {
		     std::cout << "Present mode: Immediate" << std::endl;
		     return availablePresentMode;
		   }
		 }
		std::cout << "Present mode: V-Sync" << std::endl;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D EngineSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D windowExtent)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
		{ return capabilities.currentExtent; }
		else 
		{
			VkExtent2D actualExtent = windowExtent;
			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
			return actualExtent;
		}
	}

	VkFormat EngineSwapChain::findDepthFormat(bool stencilRequired)
	{
		std::vector<VkFormat> candidates = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		if (!stencilRequired) { candidates.push_back(VK_FORMAT_D32_SFLOAT); }
		return device.findSupportedFormat(candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

}