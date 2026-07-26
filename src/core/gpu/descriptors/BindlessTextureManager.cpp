#include "core/gpu/descriptors/BindlessTextureManager.h"
#include "core/gpu/descriptors/DescriptorSetLayout.h"
#include "core/gpu/descriptors/DescriptorPool.h"

#include <stdexcept>
#include <array>
#include <utility>

namespace EngineCore
{
	BindlessTextureManager::BindlessTextureManager(EngineDevice& device) : device{ device }
	{
		createDescriptorSetLayout();
		createDescriptorPool();
		allocateDescriptorSet();
		initFallbackTexture();
	}

	BindlessTextureManager::~BindlessTextureManager()
	{
		if (fallbackSampler) vkDestroySampler(device.device(), fallbackSampler, nullptr);
		if (fallbackView) vkDestroyImageView(device.device(), fallbackView, nullptr);
		if (fallbackImage) vkDestroyImage(device.device(), fallbackImage, nullptr);
		if (fallbackMemory) vkFreeMemory(device.device(), fallbackMemory, nullptr);
	}

	void BindlessTextureManager::createDescriptorSetLayout()
	{
		VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT 
												| VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

		layoutBuilder = std::make_unique<DSetLayoutBuilder>(device);
		layoutBuilder->
			addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, MAX_BINDLESS_TEXTURES)
			.addBindingFlags(0, bindlessFlags)
			.setLayoutFlags(VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);
		layout = layoutBuilder->build();
	}

	void BindlessTextureManager::createDescriptorPool()
	{
		DescriptorPool::Builder poolBuilder(device);
		poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_TEXTURES);
		poolBuilder.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
		descriptorPool = poolBuilder.build();
	}

	void BindlessTextureManager::allocateDescriptorSet()
	{
		// allocate descriptor set with variable count
		const uint32_t variableDescCount = MAX_BINDLESS_TEXTURES;
		VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
			.descriptorSetCount = 1,
			.pDescriptorCounts = &variableDescCount
		};

		VkDescriptorSetAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = &countInfo,
			.descriptorPool = descriptorPool->getPool(),
			.descriptorSetCount = 1,
			.pSetLayouts = &getDescriptorSetLayout()
		};

		if (vkAllocateDescriptorSets(device.device(), &allocInfo, &descriptorSet) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate bindless descriptor set");
		}
	}

	const VkDescriptorSet& BindlessTextureManager::getDescriptorSet() const
	{
		return descriptorSet;
	}

	const VkDescriptorSetLayout& BindlessTextureManager::getDescriptorSetLayout() const
	{
		return layout->getDescriptorSetLayout();
	}

	uint32_t BindlessTextureManager::registerTexture(VkImageView imageView, VkSampler sampler)
	{
		uint32_t slot = 0;

		if (!freeSlots.empty())
		{
			slot = freeSlots.front();
			freeSlots.pop();
		}
		else
		{
			if (nextAvailableSlot >= MAX_BINDLESS_TEXTURES)
			{
				throw std::runtime_error("Exceeded maximum bindless texture capacity!");
			}
			slot = nextAvailableSlot++;
		}

		VkDescriptorImageInfo imageInfo{
			.sampler = sampler,
			.imageView = imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkWriteDescriptorSet descriptorWrite{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet,
			.dstBinding = 0,
			.dstArrayElement = slot,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &imageInfo
		};

		vkUpdateDescriptorSets(device.device(), 1, &descriptorWrite, 0, nullptr);

		return slot;
	}

	void BindlessTextureManager::unregisterTexture(uint32_t slotIndex)
	{
		if (slotIndex == 0) return; // Do not unregister the fallback index

		// Re-point slot to fallback texture to avoid accessing dangling views in GPU memory
		VkDescriptorImageInfo imageInfo{
			.sampler = fallbackSampler,
			.imageView = fallbackView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkWriteDescriptorSet descriptorWrite{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet,
			.dstBinding = 0,
			.dstArrayElement = slotIndex,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &imageInfo
		};

		vkUpdateDescriptorSets(device.device(), 1, &descriptorWrite, 0, nullptr);
		freeSlots.push(slotIndex);
	}

	void BindlessTextureManager::initFallbackTexture()
	{
		// 1. Create a 1x1 magenta/missing texture image & view (or use your Engine's Image wrapper)
		// ... (allocate 1x1 image, transition layout, create view & sampler) ...
		// 2. Register slot 0 as fallback
		// registerTexture(fallbackView, fallbackSampler); // Asserts slot == 0
	}

}