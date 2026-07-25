#pragma once

#include "core/types/vk.h"

#include <memory>
#include <unordered_map>

namespace EngineCore
{
	class EngineDevice;

	class DescriptorSetLayout
	{
	public:
		class Builder
		{
		public:
			Builder(EngineDevice& device) : device{ device }
			{}

			Builder& addBinding(uint32_t binding, VkDescriptorType descriptorType,
								VkShaderStageFlags stageFlags, uint32_t count = 1);
								std::unique_ptr<DescriptorSetLayout> build() const;
		private:
			EngineDevice& device;
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
		};

		DescriptorSetLayout(EngineDevice& device,
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
		~DescriptorSetLayout();
		DescriptorSetLayout(const DescriptorSetLayout&) = delete;
		DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

		VkDescriptorSetLayout getDescriptorSetLayout() const
		{
			return descriptorSetLayout;
		}

	private:
		EngineDevice& device;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

		friend class DescriptorWriter;
	};
}
