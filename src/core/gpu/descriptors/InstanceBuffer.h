#pragma once
#include "core/gpu/Descriptors.h"
#include "core/gpu/Device.h"
#include "core/gpu/descriptors/DescriptorSetLayout.h"
#include "core/gpu/descriptors/DescriptorPool.h"
#include "core/gpu/Buffer.h"
#include "core/gpu/Swapchain.h"

#include "core/types/vk.h"
#include <glm/glm.hpp>

#include <vector>
#include <queue>
#include <memory>

namespace WorldSystem
{
	class EngineNodeData;
}

namespace EngineCore
{
	class DescriptorPool;
	class DSetLayoutBuilder;
	class DescriptorSetLayout;
	class GBuffer;

	// common shader data for mesh instances
	struct alignas(16) InstanceData
	{
		glm::mat4 modelMatrix;
		glm::mat4 normalMatrix;
		uint32_t albedoTexIdx;
		uint32_t normalTexIdx;
		uint32_t roughnessTexIdx;
		uint32_t padding;
	};

	// shader-accessible storage buffer wrapper, can be used with InstanceData or other data layouts
	template <typename T>
	class InstanceBuffer
	{
	private:
		EngineDevice& device;
		std::unique_ptr<GBuffer> buffer;
		std::vector<T> instances;

	public:
		static constexpr uint32_t MAX_INSTANCES = 1000000;

		InstanceBuffer(const InstanceBuffer&) = delete;
		InstanceBuffer& operator=(const InstanceBuffer&) = delete;

		InstanceBuffer(EngineDevice& device)
			: device{ device }
		{
			// allocate an SSBO for all instance data
			VkBufferUsageFlags useFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			const auto sizeMult = MAX_INSTANCES * EngineSwapChain::MAX_FRAMES_IN_FLIGHT;
			buffer = std::make_unique<GBuffer>(device, sizeof(T), sizeMult, useFlags, memFlags, 0);
			buffer->map();
		}

		~InstanceBuffer()
		{}

		void addInstanceData(const T& d)
		{
			instances.push_back(d);
		}

		void pushBufferToGPU(uint32_t frameIndex)
		{
			assert(instances.size() <= MAX_INSTANCES && "instance count exceeds MAX_INSTANCES");
			const size_t frameOffsetBytes = frameIndex * MAX_INSTANCES * sizeof(T);
			buffer->writeToBuffer((void*)instances.data(), instances.size() * sizeof(T), frameOffsetBytes);
			instances.clear();
		}

		VkDeviceAddress getDeviceAddress(uint32_t currentFrame) const
		{
			VkBufferDeviceAddressInfo addressInfo{
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.buffer = buffer->getBuffer()
			};

			VkDeviceAddress baseAddress = vkGetBufferDeviceAddress(device.device(), &addressInfo);
			// offset the pointer to the start of the current frame's slice
			const size_t frameOffsetBytes = currentFrame * MAX_INSTANCES * sizeof(T);
			return baseAddress + frameOffsetBytes;
		}

	};

}