#include "core/gpu/descriptors/DescriptorSetLayout.h"
#include "core/gpu/Device.h"

#include "core/types/vk.h"

#include <cassert>
#include <stdexcept>

namespace EngineCore
{
	// *************** Descriptor Set Layout Builder *********************

	DSetLayoutBuilder& DSetLayoutBuilder::addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count)
	{
		assert(bindings.count(binding) == 0 && "Binding already in use");
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count; // array length
		layoutBinding.stageFlags = stageFlags;
		bindings[binding] = layoutBinding;
		return *this;
	}

	std::unique_ptr<DescriptorSetLayout> DSetLayoutBuilder::build() const
	{
		return std::make_unique<DescriptorSetLayout>(device, bindings, bindingFlags, layoutFlags);
	}

	DSetLayoutBuilder& DSetLayoutBuilder::addBindingFlags(uint32_t binding, VkDescriptorBindingFlags flags)
	{
		bindingFlags[binding] = flags;
		return *this;
	}

	DSetLayoutBuilder& DSetLayoutBuilder::setLayoutFlags(VkDescriptorSetLayoutCreateFlags flags)
	{
		layoutFlags = flags;
		return *this;
	}

	// *************** Descriptor Set Layout *********************

	DescriptorSetLayout::DescriptorSetLayout(EngineDevice& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings, 
						std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlags, VkDescriptorSetLayoutCreateFlags layoutFlags)
		: device{ device }, bindings{ bindings }
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
		std::vector<VkDescriptorBindingFlags> setLayoutBindingFlags{};
		for (const auto& kv : bindings)
		{
			setLayoutBindings.push_back(kv.second);
			// if flags were specified for this binding, use them - otherwise default to 0
			auto it = bindingFlags.find(kv.first);
			setLayoutBindingFlags.push_back(it != bindingFlags.end() ? it->second : 0);
		}

		// connect binding flags
		VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
		flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		flagsInfo.bindingCount = static_cast<uint32_t>(setLayoutBindingFlags.size());
		flagsInfo.pBindingFlags = setLayoutBindingFlags.data();

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.pNext = &flagsInfo;
		descriptorSetLayoutInfo.flags = layoutFlags;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(
			device.device(),
			&descriptorSetLayoutInfo,
			nullptr,
			&descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayout, nullptr);
	}

}