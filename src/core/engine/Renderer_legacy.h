#pragma once

#include "Core/Swapchain.h"

#include <memory>
#include <vector>
#include <cassert>

namespace EngineCore
{
	// forward-declarations
	class EngineWindow;
	class EngineDevice;
	class EngineRenderSettings;

	class Renderer
	{
	public:
		Renderer(EngineWindow& windowIn, EngineDevice& deviceIn, EngineRenderSettings& renderSettingsIn);
		~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		
		VkRenderPass getSwapchainRenderPass() const;
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
		float getAspectRatio() const;

		// returns a command buffer to record commands into
		VkCommandBuffer beginFrame();
		// submit command buffer to finalize the frame
		void endFrame();
		void beginSwapchainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapchainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapchain();

		EngineWindow& window;
		EngineDevice& device;
		EngineRenderSettings& renderSettings;
		std::unique_ptr<EngineSwapChain> swapchain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 };
		bool isFrameStarted{ false };
	};

} // namespace