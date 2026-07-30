#pragma once

#include "core/types/vk.h"

#include <memory>

namespace EngineCore
{
	class EngineDevice;
	class Fence;
	class GBuffer;

	enum class ESingleTimeCommandsState : uint32_t { NOT_BEGUN, BEGUN, SUBMITTED };

	class SingleTimeCommands
	{
	public:
		SingleTimeCommands(EngineDevice& device);
		~SingleTimeCommands();

		VkCommandBuffer begin();
		void submit();
		void copyBuffer(GBuffer& src, GBuffer& dst, VkDeviceSize size);
		// if this returns true, it is safe to call begin() again, or destroy the object
		bool finished() const;
		void reset();


	private:
		EngineDevice& device;
		std::unique_ptr<Fence> fence;
		VkCommandBuffer cmdBuffer;
		ESingleTimeCommandsState state = ESingleTimeCommandsState::NOT_BEGUN;
	};
}