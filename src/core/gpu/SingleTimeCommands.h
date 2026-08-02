#pragma once

#include "core/types/vk.h"

#include <memory>
#include <mutex>

namespace EngineCore
{
	class EngineDevice;
	class AsyncCommandBuffer;
	class Fence;
	class GBuffer;

	enum class ESingleTimeCommandsState : uint32_t { NOT_BEGUN, BEGUN, SUBMITTED };

	class SingleTimeCommands
	{
	public:
		SingleTimeCommands(EngineDevice& device);
		~SingleTimeCommands();

		VkCommandBuffer begin(std::unique_lock<std::mutex>& lock);
		void submit();
		void copyBuffer(GBuffer& src, GBuffer& dst, VkDeviceSize size);
		// if this returns true, it is safe to destroy the object
		bool finished() const;


	private:
		EngineDevice& device;
		std::shared_ptr<AsyncCommandBuffer> cmdBuffer;
		ESingleTimeCommandsState state = ESingleTimeCommandsState::NOT_BEGUN;
	};
}