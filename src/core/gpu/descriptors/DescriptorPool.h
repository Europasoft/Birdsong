#pragma once

#include "core/types/vk.h"

#include <memory>
#include <vector>

namespace EngineCore
{
	class EngineDevice;
	class DescriptorSetLayout;

	class DescriptorPool
	{
	public:
		class Builder
		{
		public:
			Builder(EngineDevice& device) : device{ device }
			{}

			Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
			Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
			Builder& setMaxSets(uint32_t count);
			std::unique_ptr<DescriptorPool> build() const;

		private:
			EngineDevice& device;
			std::vector<VkDescriptorPoolSize> poolSizes{};
			uint32_t maxSets = 1000;
			VkDescriptorPoolCreateFlags poolFlags = 0;
		};

		DescriptorPool(EngineDevice& device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,
			const std::vector<VkDescriptorPoolSize>& poolSizes);
		~DescriptorPool();
		DescriptorPool(const DescriptorPool&) = delete;
		DescriptorPool& operator=(const DescriptorPool&) = delete;

		bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;
		void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

		void resetPool();
		VkDescriptorPool getPool() const { return descriptorPool; }

	private:
		EngineDevice& device;
		VkDescriptorPool descriptorPool;

		friend class DescriptorWriter;
	};

	class DescriptorWriter
	{
	public:
		DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

		DescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		DescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, uint32_t arrSize = 1, uint32_t dstArrayElement = 0);

		bool build(VkDescriptorSet& set);
		void overwrite(VkDescriptorSet& set);

	private:
		DescriptorSetLayout& setLayout;
		DescriptorPool& pool;
		std::vector<VkWriteDescriptorSet> writes;
	};
}
