#include "Core/Render/Renderer.h"

#include "Core/Window.h"
#include "Core/GPU/Device.h"
#include "Core/EngineSettings.h"

#include <stdexcept>
#include <array>
#include <cassert>

namespace EngineCore
{
	Renderer::Renderer(EngineWindow& window, EngineDevice& device, EngineRenderSettings& renderSettings)
							: window{window}, device{device}, renderSettings{renderSettings}
	{
		create();
		createCommandBuffers();
	}

	Renderer::~Renderer() { freeCommandBuffers(); }

	void Renderer::createCommandBuffers()
	{
		commandBuffers.resize(EngineSwapChain::MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
		if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate command buffers");
		}
	}

	void Renderer::freeCommandBuffers()
	{
		vkFreeCommandBuffers(device.device(), device.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		commandBuffers.clear();
	}

	void Renderer::create()
	{
		currentFrameIndex = 0;
		createSwapchain();
		createAttachments();
		if (swapchainCreatedCallback) { swapchainCreatedCallback(); }
	}

	void Renderer::createSwapchain()
	{
		auto extent = window.getExtent();
		while (extent.width == 0 || extent.height == 0)
		{
			extent = window.getExtent();
			glfwWaitEvents(); // this happens during resize or minimization of the glfw window
		}
		vkDeviceWaitIdle(device.device());

		if (swapchain == nullptr)
		{
			swapchain = std::make_unique<EngineSwapChain>(device, extent);
		}
		else
		{
			std::shared_ptr<EngineSwapChain> oldSwapChain = std::move(swapchain);
			swapchain = std::make_unique<EngineSwapChain>(device, extent, oldSwapChain);
			if (!oldSwapChain->compareSwapFormats(*swapchain.get())) { throw std::runtime_error("swap chain image or depth format changed unexpectedly"); }
		}
	}

	void Renderer::createAttachments() 
	{
		// clear previous attachments
		attachments.clear();

		AttachmentProperties color = swapchain->getAttachmentProperties();
		color.type = AttachmentType::COLOR;
		color.samples = renderSettings.sampleCountMSAA;

		AttachmentProperties resolve = color;
		resolve.type = AttachmentType::RESOLVE;
		resolve.samples = VK_SAMPLE_COUNT_1_BIT;

		AttachmentProperties depth = color;
		depth.type = AttachmentType::DEPTH;
		depth.format = swapchain->getDepthFormat();

		AttachmentProperties depthResolve = depth;
		depthResolve.type = AttachmentType::DEPTH_STENCIL_RESOLVE;
		depthResolve.samples = VK_SAMPLE_COUNT_1_BIT;

		// create attachments and store pointers for rendering
		colorAttachment = &addAttachment(color, false, false);
		colorResolveAttachment = &addAttachment(resolve, false, true);
		depthAttachment = &addAttachment(depth, false, false);
		depthResolveAttachment = &addAttachment(depthResolve, false, true);

		// bound to descriptor set to be sampled in fx pass
		fxPassInputImageViews = colorResolveAttachment->getImageViews();
		fxPassInputDepthImageViews = depthAttachment->getImageViews();

		// setup rendering formats for VK_KHR_dynamic_rendering pipeline creation
		basePassFormats.colorFormats = { color.format };
		basePassFormats.depthFormat = depth.format;

		fxPassFormats.colorFormats = { swapchain->getSwapChainImageFormat() };
		fxPassFormats.depthFormat = depthResolve.format;
	}

	void Renderer::transitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, 
										VkImageLayout oldLayout, VkImageLayout newLayout,
										VkAccessFlags srcAccess, VkAccessFlags dstAccess,
										VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
										VkImageAspectFlags aspectMask)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = aspectMask;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;

		vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void Renderer::beginRenderingBase(VkCommandBuffer cmdBuffer)
	{
		assert(isFrameStarted && "failed to begin rendering, no frame in progress");

		auto colorImages = colorAttachment->getImages();
		auto resolveImages = colorResolveAttachment->getImages();
		auto depthImages = depthAttachment->getImages();
		auto depthResolveImages = depthResolveAttachment->getImages();

		auto colorViews = colorAttachment->getImageViews();
		auto resolveViews = colorResolveAttachment->getImageViews();
		auto depthViews = depthAttachment->getImageViews();
		auto depthResolveViews = depthResolveAttachment->getImageViews();

		// transition all attachments from UNDEFINED to their required layouts
		// color attachment (MSAA)
		transitionImageLayout(cmdBuffer, colorImages[currentImageIndex],
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		// color resolve attachment
		transitionImageLayout(cmdBuffer, resolveImages[currentImageIndex],
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		// depth attachment (MSAA)
		transitionImageLayout(cmdBuffer, depthImages[currentImageIndex],
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

		// depth resolve attachment
		transitionImageLayout(cmdBuffer, depthResolveImages[currentImageIndex],
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

		// color attachment with MSAA resolve
		VkRenderingAttachmentInfo colorAttachmentInfo{};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachmentInfo.imageView = colorViews[currentImageIndex];
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue.color = {{0.01f, 0.01f, 0.01f, 1.0f}};
		// MSAA resolve
		colorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
		colorAttachmentInfo.resolveImageView = resolveViews[currentImageIndex];
		colorAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// depth attachment with resolve
		VkRenderingAttachmentInfo depthAttachmentInfo{};
		depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachmentInfo.imageView = depthViews[currentImageIndex];
		depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachmentInfo.clearValue.depthStencil = {1.0f, 0};
		// depth resolve
		depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_MIN_BIT;
		depthAttachmentInfo.resolveImageView = depthResolveViews[currentImageIndex];
		depthAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = {{0, 0}, swapchain->getExtent()};
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;

		// set viewport and scissor
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchain->getExtent().width);
		viewport.height = static_cast<float>(swapchain->getExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{{0, 0}, swapchain->getExtent()};

		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		vkCmdBeginRendering(cmdBuffer, &renderingInfo);
	}

	void Renderer::beginRenderingFx(VkCommandBuffer cmdBuffer)
	{
		assert(isFrameStarted && "failed to begin rendering, no frame in progress");

		auto resolveImages = colorResolveAttachment->getImages();
		auto depthResolveViews = depthResolveAttachment->getImageViews();
		auto swapchainViews = swapchain->getSwapchainAttachment().getImageViews();

		// transition the resolved color attachment to shader read for sampling in FX pass
		transitionImageLayout(cmdBuffer, 
			resolveImages[currentImageIndex],
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		// transition swapchain image to color attachment
		transitionImageLayout(cmdBuffer, 
			swapchain->getSwapChainImages()[currentImageIndex],
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		// swapchain color attachment
		VkRenderingAttachmentInfo colorAttachmentInfo{};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachmentInfo.imageView = swapchainViews[currentImageIndex];
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

		// depth attachment (reuse from base pass)
		VkRenderingAttachmentInfo depthAttachmentInfo{};
		depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachmentInfo.imageView = depthResolveViews[currentImageIndex];
		depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = {{0, 0}, swapchain->getExtent()};
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;

		// set viewport and scissor
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchain->getExtent().width);
		viewport.height = static_cast<float>(swapchain->getExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{{0, 0}, swapchain->getExtent()};

		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		vkCmdBeginRendering(cmdBuffer, &renderingInfo);
	}

	void Renderer::endRendering(VkCommandBuffer cmdBuffer)
	{
		assert(isFrameStarted && "failed to end rendering, no frame in progress");
		vkCmdEndRendering(cmdBuffer);
	}

	VkCommandBuffer Renderer::beginFrame() 
	{
		assert(!isFrameStarted && "beginFrame failed, frame already in progress");
		
		auto result = swapchain->acquireNextImage(&currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) 
		{ 
			create(); // recreate swapchain and renderpasses
			return nullptr; 
		} 
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { throw std::runtime_error("failed to acquire swapchain image"); }

		isFrameStarted = true;
		auto commandBuffer = getCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) { throw std::runtime_error("failed to begin recording command buffer"); }

		return commandBuffer;
	}

	void Renderer::endFrame() 
	{
		assert(isFrameStarted && "endFrame failed, no frame in progress");

		auto commandBuffer = getCurrentCommandBuffer();

		// transition swapchain image to present layout
		transitionImageLayout(commandBuffer,
			swapchain->getSwapChainImages()[currentImageIndex],
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{ throw std::runtime_error("failed to record command buffer"); }

		auto result = swapchain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized())
		{
			window.resetWindowResizedFlag();
			create();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swapchain image");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % EngineSwapChain::MAX_FRAMES_IN_FLIGHT;
	}


	//VkRenderPass Renderer::getSwapchainRenderPass() const { return swapchain->getRenderPass(); }

	float Renderer::getSwapchainAspectRatio() const { return swapchain->getExtentAspectRatio(); }

}