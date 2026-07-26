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
		DescriptorSetLayout(EngineDevice& device,std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings,
						std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlags, VkDescriptorSetLayoutCreateFlags layoutFlags);
		~DescriptorSetLayout();
		DescriptorSetLayout(const DescriptorSetLayout&) = delete;
		DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

		const VkDescriptorSetLayout& getDescriptorSetLayout() const
		{
			return descriptorSetLayout;
		}

	private:
		EngineDevice& device;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

		friend class DescriptorWriter;
	};

	class DSetLayoutBuilder
	{
	public:
		DSetLayoutBuilder(EngineDevice& device) : device{ device }
		{}

		DSetLayoutBuilder& addBinding(uint32_t binding, VkDescriptorType descriptorType,
			VkShaderStageFlags stageFlags, uint32_t count = 1);
		std::unique_ptr<DescriptorSetLayout> build() const;
		// optional descriptor binding flags, e.g. PARTIALLY_BOUND, VARIABLE_DESCRIPTOR_COUNT
		DSetLayoutBuilder& addBindingFlags(uint32_t binding, VkDescriptorBindingFlags flags);

		// optional descriptor set layout create flags, e.g. UPDATE_AFTER_BIND_POOL_BIT
		DSetLayoutBuilder& setLayoutFlags(VkDescriptorSetLayoutCreateFlags flags);
	private:
		EngineDevice& device;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
		std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlags{};
		VkDescriptorSetLayoutCreateFlags layoutFlags{ 0 };
	};
}
