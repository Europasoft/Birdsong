#include "core/gpu/SingleTimeCommands.h"
#include "core/gpu/Device.h"
#include "core/gpu/Buffer.h"

#include <cassert>

namespace EngineCore
{
	SingleTimeCommands::SingleTimeCommands(EngineDevice& device)
		: device(device)
	{
	}

	SingleTimeCommands::~SingleTimeCommands()
	{
	}

	VkCommandBuffer SingleTimeCommands::begin(std::unique_lock<std::mutex>& lock)
	{
		assert(state != ESingleTimeCommandsState::SUBMITTED && "cannot call begin() again when already submitted");
		assert(state != ESingleTimeCommandsState::BEGUN && "cannot call begin() twice");
		cmdBuffer = device.getAsyncCommandDispatcher().startNewCommandBuffer(lock);
		state = ESingleTimeCommandsState::BEGUN;
		return cmdBuffer->get();
	}

	void SingleTimeCommands::submit()
	{
		assert(state == ESingleTimeCommandsState::BEGUN && "must call begin() before submit(), cannot call twice");
		cmdBuffer->markForSubmit();
		state = ESingleTimeCommandsState::SUBMITTED;
	}

	bool SingleTimeCommands::finished() const
	{
		assert(state == ESingleTimeCommandsState::SUBMITTED && "must call submit() before checking status");
		return cmdBuffer->finished();
	}

	void SingleTimeCommands::copyBuffer(GBuffer& src, GBuffer& dst, VkDeviceSize size)
	{
		assert(state == ESingleTimeCommandsState::BEGUN && "copyBuffer() must be called after begin()");
		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(cmdBuffer->get(), src.getBuffer(), dst.getBuffer(), 1, &copyRegion);
	}
	
}

