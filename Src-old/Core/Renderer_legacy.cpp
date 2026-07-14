#include "Core/Renderer_legacy.h"

#include "Core/Window.h"
#include "Core/GPU/Device.h"
#include "Core/EngineSettings.h"

#include <stdexcept>
#include <array>
#include <cassert>

namespace EngineCore
{
	Renderer::Renderer(EngineWindow& windowIn, EngineDevice& deviceIn, EngineRenderSettings& renderSettingsIn)
							: window{windowIn}, device{deviceIn}, renderSettings{renderSettingsIn}
	{
		recreateSwapchain();
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
		vkFreeCommandBuffers(device.device(), device.getCommandPool(),
			static_cast<float>(commandBuffers.size()), commandBuffers.data());
		commandBuffers.clear();
	}

	void Renderer::recreateSwapchain()
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
			swapchain = std::make_unique<EngineSwapChain>(device, extent, renderSettings.sampleCountMSAA);
		}
		else
		{
			std::shared_ptr<EngineSwapChain> oldSwapChain = std::move(swapchain);
			swapchain = std::make_unique<EngineSwapChain>(device, extent, renderSettings.sampleCountMSAA, oldSwapChain);
			if (!oldSwapChain->compareSwapFormats(*swapchain.get()))
			{
				throw std::runtime_error("swap chain image or depth format changed unexpectedly");
			}
		}
	}

	VkCommandBuffer Renderer::beginFrame() 
	{
		assert(!isFrameStarted && "beginFrame failed, frame already in progress");
		
		auto result = swapchain->acquireNextImage(&currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{ recreateSwapchain(); return nullptr; }
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{ throw std::runtime_error("failed to acquire swapchain image"); }

		isFrameStarted = true;

		auto commandBuffer = getCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{ throw std::runtime_error("failed to begin recording command buffer"); }

		return commandBuffer;
	}

	void Renderer::endFrame() 
	{
		assert(isFrameStarted && "endFrame failed, no frame in progress");

		auto commandBuffer = getCurrentCommandBuffer();

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{ throw std::runtime_error("failed to record command buffer"); }

		auto result = swapchain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized())
		{
			window.resetWindowResizedFlag();
			recreateSwapchain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swapchain image");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % EngineSwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void Renderer::beginSwapchainRenderPass(VkCommandBuffer commandBuffer) 
	{
		assert(isFrameStarted && "beginSwapchainRenderPass failed, no frame in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "cannot begin renderpass on commandbuffer from other frame");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = swapchain->getRenderPass();
		renderPassInfo.framebuffer = swapchain->getFrameBuffer(currentImageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // inline = no secondary cmdbuffers

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(swapchain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, swapchain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void Renderer::endSwapchainRenderPass(VkCommandBuffer commandBuffer) 
	{
		assert(isFrameStarted && "endSwapchainRenderPass failed, no frame in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "cannot end renderpass on commandbuffer from other frame");
		vkCmdEndRenderPass(commandBuffer);	
	}

	VkRenderPass Renderer::getSwapchainRenderPass() const { return swapchain->getRenderPass(); }

	float Renderer::getAspectRatio() const { return swapchain->getExtentAspectRatio(); }

} // namespace