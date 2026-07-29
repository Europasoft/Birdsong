#pragma once

#include "core/gpu/Descriptors.h"
#include "core/gpu/Device.h"
#include "core/types/vk.h"

#include <vector>
#include <queue>
#include <memory>

namespace EngineCore
{
	class DescriptorPool;
	class DSetLayoutBuilder;
	class DescriptorSetLayout;
	class Image;

	class BindlessTextureManager
	{
	public:
		static constexpr uint32_t MAX_BINDLESS_TEXTURES = 128000;

		BindlessTextureManager(EngineDevice& device);
		~BindlessTextureManager();

		BindlessTextureManager(const BindlessTextureManager&) = delete;
		BindlessTextureManager& operator=(const BindlessTextureManager&) = delete;

		// registers a texture view + sampler and returns its index in the global array
		uint32_t registerTexture(VkImageView imageView, VkSampler sampler);
		uint32_t registerTexture(const std::unique_ptr<Image>& image);

		// frees a slot index so it can be reused by a future texture
		void unregisterTexture(uint32_t slotIndex);

		const VkDescriptorSet& getDescriptorSet() const;
		const VkDescriptorSetLayout& getDescriptorSetLayout() const;

	private:
		void createDescriptorSetLayout();
		void createDescriptorPool();
		void allocateDescriptorSet();
		void initFallbackTexture();

		EngineDevice& device;

		std::unique_ptr<DescriptorPool> descriptorPool;
		std::unique_ptr<DSetLayoutBuilder> layoutBuilder;
		std::unique_ptr<DescriptorSetLayout> layout;
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };

		uint32_t nextAvailableSlot{ 1 }; // reserve slot 0 as the permanent fallback index
		std::queue<uint32_t> freeSlots;

		// default dummy texture for index 0 (fallback for unbound/loading textures)
		std::unique_ptr<Image> fallbackImage;

	};

}