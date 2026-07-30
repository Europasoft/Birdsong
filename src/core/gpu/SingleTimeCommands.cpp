#include "core/gpu/SingleTimeCommands.h"
#include "core/gpu/Device.h"
#include "core/gpu/Buffer.h"

#include <cassert>

namespace EngineCore
{
	SingleTimeCommands::SingleTimeCommands(EngineDevice& device)
		: device(device)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = device.getCommandPool();
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(device.device(), &allocInfo, &cmdBuffer);
	}

	SingleTimeCommands::~SingleTimeCommands()
	{
		assert((not fence) || fence->wasSignaled() && "SingleTimeCommands destroyed before commands finished");
		vkFreeCommandBuffers(device.device(), device.getCommandPool(), 1, &cmdBuffer);
	}

	VkCommandBuffer SingleTimeCommands::begin()
	{
		assert(state != ESingleTimeCommandsState::SUBMITTED && "cannot call begin() again when already submitted");
		if (state == ESingleTimeCommandsState::NOT_BEGUN)
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(cmdBuffer, &beginInfo);
			state = ESingleTimeCommandsState::BEGUN;
		}
		return cmdBuffer;
	}

	void SingleTimeCommands::submit()
	{
		assert(state == ESingleTimeCommandsState::BEGUN && "must call begin() before submit(), cannot call twice");
		vkEndCommandBuffer(cmdBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		fence = std::make_unique<Fence>(device);
		vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, fence->getFence());
		state = ESingleTimeCommandsState::SUBMITTED;
	}

	bool SingleTimeCommands::finished() const
	{
		assert(fence && state != ESingleTimeCommandsState::BEGUN && "must call submit() before checking status");
		return fence->wasSignaled() || state == ESingleTimeCommandsState::NOT_BEGUN;
	}

	void SingleTimeCommands::copyBuffer(GBuffer& src, GBuffer& dst, VkDeviceSize size)
	{
		assert(state == ESingleTimeCommandsState::BEGUN && "copyBuffer() must be called after begin()");
		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(cmdBuffer, src.getBuffer(), dst.getBuffer(), 1, &copyRegion);
	}

	void SingleTimeCommands::reset()
	{
		assert(state == ESingleTimeCommandsState::SUBMITTED && "reset() must be called after submit()");
		vkResetCommandBuffer(cmdBuffer, 0);
		fence.release();
		state = ESingleTimeCommandsState::NOT_BEGUN;
	}
	
}

