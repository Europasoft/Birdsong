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
		createSwapchain();
		createRenderpasses();
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

	void Renderer::createRenderpasses() 
	{
		using Load = AttachmentLoadOp;
		using Store = AttachmentStoreOp;
		using Use = AttachmentUse;

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


		const auto& colorAttachment = addAttachment(color, false, false);
		const auto& colorResolveAttachment = addAttachment(resolve, false, true);
		const auto& depthAttachment = addAttachment(depth, false, false);
		const auto& depthResolveAttachment = addAttachment(depthResolve, false, true);

		const auto& swapchainAttachment = swapchain->getSwapchainAttachment();
		//const auto& depthFxAttachment = addAttachment(depthResolve, false, false);
		
		const std::vector<Use> baseUses =
		{
				Use(colorAttachment, Load::CLEAR, Store::STORE,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),

				Use(colorResolveAttachment, Load::CLEAR, Store::STORE,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),

				Use(depthAttachment, Load::CLEAR, Store::STORE,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),

				Use(depthResolveAttachment, Load::CLEAR, Store::STORE,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		};

		// bound to descriptor set to be sampled in fx pass
		fxPassInputImageViews = colorResolveAttachment.getImageViews();
		fxPassInputDepthImageViews = depthAttachment.getImageViews();

		const std::vector<Use> fxUses =
		{
				Use(swapchainAttachment, Load::CLEAR, Store::STORE,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR),

				Use(depthResolveAttachment, Load::LOAD, Store::STORE,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
				AttachmentType::DEPTH_STENCIL) // reused with different type flag
		};

		baseRenderpass = std::make_unique<Renderpass>(device, baseUses, color.extent, color.imageCount);
		fxRenderpass = std::make_unique<Renderpass>(device, fxUses, color.extent, color.imageCount);

	}

	void Renderer::beginRenderpassBase(VkCommandBuffer cmdBuffer) { baseRenderpass->begin(cmdBuffer, currentImageIndex); }
	void Renderer::beginRenderpassFx(VkCommandBuffer cmdBuffer) { fxRenderpass->begin(cmdBuffer, currentImageIndex); }

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