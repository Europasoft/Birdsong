#include "core/gpu/descriptors/BindlessTextureManager.h"
#include "core/gpu/descriptors/DescriptorSetLayout.h"
#include "core/gpu/descriptors/DescriptorPool.h"
#include "core/gpu/Image.h"
#include "core/types/CommonTypes.h"

#include <stdexcept>
#include <array>
#include <utility>
#include <cassert>
#include <iostream>

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
				throw std::runtime_error("exceeded maximum bindless texture capacity");
			}
			slot = nextAvailableSlot++;
		}

		VkDescriptorImageInfo imageInfo
		{
			.sampler = sampler,
			.imageView = imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkWriteDescriptorSet descriptorWrite
		{
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
		// re-point slot to fallback texture to avoid accessing dangling views in GPU memory
		VkDescriptorImageInfo imageInfo
		{
			.sampler = fallbackImage->sampler,
			.imageView = fallbackImage->getView(),
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
		VkImageCreateInfo imageInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.extent = {1, 1, 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		
		try 
		{
			fallbackImage = std::make_unique<Image>(device, makePath("textures/missing.png"));
		}
		catch (...)
		{
			std::cout << "Failed to load the fallback texture - defaulting to solid color\n";
			// create 1x1 image
			fallbackImage = std::make_unique<Image>(device, imageInfo);
			std::array<uint8_t, 4> rgba = { 0xFF, 0x00, 0xFF, 0xFF };
			GBuffer stagingBuffer
			{
				device, rgba.size(), 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			};
			// map temporary buffer to host and copy to it
			stagingBuffer.map(rgba.size());
			memcpy(stagingBuffer.getMappedMemory(), rgba.data(), static_cast<size_t>(rgba.size()));
			stagingBuffer.unmap();
			// push to final image (also handles layout transitions)
			fallbackImage->copyBufferToImage(stagingBuffer, imageInfo.extent.width, imageInfo.extent.height, 1);

			// create view and sampler
			fallbackImage->updateView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
			Image::createSampler(fallbackImage->sampler, device, 1.f);
		}

		// pre-fill every slot with fallback texture
		std::vector<VkDescriptorImageInfo> imageInfos(MAX_BINDLESS_TEXTURES, VkDescriptorImageInfo
			{
				.sampler = fallbackImage->sampler,
				.imageView = fallbackImage->getView(),
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			});

		VkWriteDescriptorSet writeAll
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = MAX_BINDLESS_TEXTURES,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = imageInfos.data()
		};

		vkUpdateDescriptorSets(device.device(), 1, &writeAll, 0, nullptr);
	}

}