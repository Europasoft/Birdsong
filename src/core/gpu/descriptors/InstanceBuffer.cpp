#include "core/gpu/descriptors/InstanceBuffer.h"
#include "core/gpu/descriptors/DescriptorSetLayout.h"
#include "core/gpu/descriptors/DescriptorPool.h"
#include "core/gpu/Buffer.h"
#include "core/gpu/Swapchain.h"

namespace EngineCore
{
	InstanceBuffer::InstanceBuffer(EngineDevice& device)
		: device{ device }
	{
		// allocate an SSBO for all instance data
		VkBufferUsageFlags useFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		const auto sizeMult = MAX_INSTANCES * EngineSwapChain::MAX_FRAMES_IN_FLIGHT;
		buffer = std::make_unique<GBuffer>(device, sizeof(InstanceData), sizeMult, useFlags, memFlags, 0);
		buffer->map();
	}

	InstanceBuffer::~InstanceBuffer()
	{
	}

	void InstanceBuffer::addInstanceData(const InstanceData& d)
	{
		instances.push_back(d);
	}

	void InstanceBuffer::pushBufferToGPU(uint32_t frameIndex)
	{
		assert(instances.size() <= MAX_INSTANCES && "instance count exceeds MAX_INSTANCES");
		const size_t frameOffsetBytes = frameIndex * MAX_INSTANCES * sizeof(InstanceData);
		buffer->writeToBuffer((void*)instances.data(), instances.size() * sizeof(InstanceData), frameOffsetBytes);
		instances.clear();
	}

	VkDeviceAddress InstanceBuffer::getDeviceAddress(uint32_t currentFrame) const
	{
		VkBufferDeviceAddressInfo addressInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = buffer->getBuffer()
		};

		VkDeviceAddress baseAddress = vkGetBufferDeviceAddress(device.device(), &addressInfo);
		// offset the pointer to the start of the current frame's slice
		const size_t frameOffsetBytes = currentFrame * MAX_INSTANCES * sizeof(InstanceData);
		return baseAddress + frameOffsetBytes;
	}

}