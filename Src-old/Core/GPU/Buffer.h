#pragma once

#include "Core/GPU/Device.h"

namespace EngineCore 
{
	class GBuffer 
	{
	public:
		GBuffer(EngineDevice& device, VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags,
				VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment = 1);
		~GBuffer();

		GBuffer(const GBuffer&) = delete;
		GBuffer& operator=(const GBuffer&) = delete;

		// calls vkMapMemory for this buffer, whole range by default, starting at 0 (bytes)
		VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void unmap();

		void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		// flushes buffer memory to make it visible to the device (GPU), only required for non-coherent memory
		VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		// invalidate buffer memory to make it visible to the device (GPU), only required for non-coherent memory
		VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		// copies "instanceSize" bytes to the mapped buffer at offset of index*alignmentSize
		void writeToIndex(void* data, int index);
		// flush the memory range at index*alignmentSize in the buffer to make it visible to the device
		VkResult flushIndex(int index);
		VkDescriptorBufferInfo descriptorInfoForIndex(int index);
		VkResult invalidateIndex(int index);

		// returns the underlying VkBuffer handle
		VkBuffer getBuffer() const { return buffer; }
		void* getMappedMemory() const { return mapped; }
		uint32_t getInstanceCount() const { return instanceCount; }
		VkDeviceSize getInstanceSize() const { return instanceSize; }
		VkDeviceSize getAlignmentSize() const { return alignmentSize; }
		VkBufferUsageFlags getUsageFlags() const { return usageFlags; }
		VkMemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; }
		VkDeviceSize getBufferSize() const { return bufferSize; }

	private:
		// minimum instance size required to be compatible with device minOffsetAlignment
		static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

		EngineDevice& device;
		void* mapped = nullptr;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;

		VkDeviceSize bufferSize;
		uint32_t instanceCount;
		VkDeviceSize instanceSize;
		VkDeviceSize alignmentSize;
		VkBufferUsageFlags usageFlags;
		VkMemoryPropertyFlags memoryPropertyFlags;
	};

}
