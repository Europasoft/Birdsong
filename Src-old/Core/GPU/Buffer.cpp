
#include "Core/GPU/Buffer.h"

#include <cassert>
#include <cstring>

namespace EngineCore 
{
	VkDeviceSize GBuffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) 
	{
		if (minOffsetAlignment > 0) 
		{
			return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
		}
		return instanceSize;
	}

	GBuffer::GBuffer(EngineDevice& device, VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags, 
						VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment)
				: device{ device }, instanceSize{ instanceSize }, instanceCount{ instanceCount }, 
				usageFlags{ usageFlags }, memoryPropertyFlags{ memoryPropertyFlags } 
	{
		alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
		bufferSize = alignmentSize * instanceCount;
		device.createBuffer(bufferSize, usageFlags, memoryPropertyFlags, buffer, memory);
	}

	GBuffer::~GBuffer() 
	{
		unmap();
		vkDestroyBuffer(device.device(), buffer, nullptr);
		vkFreeMemory(device.device(), memory, nullptr);
	}

	VkResult GBuffer::map(VkDeviceSize size, VkDeviceSize offset) 
	{
		// this maps the memory range to the host, making it CPU-accessible
		assert(buffer && memory && "cannot map uninitialized buffer");
		return vkMapMemory(device.device(), memory, offset, size, 0, &mapped);
	}

	// vkUnmapMemory can't fail
	void GBuffer::unmap() 
	{
		if (mapped) {
			vkUnmapMemory(device.device(), memory);
			mapped = nullptr;
		}
	}

	void GBuffer::writeToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) 
	{
		assert(mapped && "cannot copy to unmapped buffer");

		if (size == VK_WHOLE_SIZE) 
		{ memcpy(mapped, data, bufferSize); }
		else 
		{
			char* memOffset = (char*)mapped;
			memOffset += offset;
			memcpy(memOffset, data, size);
		}
	}

	VkResult GBuffer::flush(VkDeviceSize size, VkDeviceSize offset) 
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size; // bytes to flush, starting at offset
		return vkFlushMappedMemoryRanges(device.device(), 1, &mappedRange);
	}

	VkResult GBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) 
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(device.device(), 1, &mappedRange);
	}

	VkDescriptorBufferInfo GBuffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) 
	{
		return VkDescriptorBufferInfo
		{
			buffer,
			offset,
			size,
		};
	}

	void GBuffer::writeToIndex(void* data, int index) { writeToBuffer(data, instanceSize, index * alignmentSize); }

	VkResult GBuffer::flushIndex(int index) { return flush(alignmentSize, index * alignmentSize); }

	VkDescriptorBufferInfo GBuffer::descriptorInfoForIndex(int index) 
	{
		return descriptorInfo(alignmentSize, index * alignmentSize);
	}

	VkResult GBuffer::invalidateIndex(int index) 
	{
		return invalidate(alignmentSize, index * alignmentSize);
	}

}