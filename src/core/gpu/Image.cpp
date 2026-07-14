#include "core/gpu/Image.h"
#include "core/gpu/Device.h"
#include "core/gpu/Buffer.h"
#include <cassert>
#include <stdexcept>

// image importer, can only be defined in one (source) file
#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

namespace EngineCore
{
	Image::Image(EngineDevice& device, VkImage image)
		: device{ device }, image{ image } {}

	Image::Image(EngineDevice& device, const std::string& path)
		: device{ device }
	{
		loadFromDisk(path);
		updateView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
		createSampler(sampler, device, 1.f); // TODO: need to check if device supports the anisotropy level!
		setDebugName(path);
	}

	Image::Image(EngineDevice& device, VkImageCreateInfo info, VkMemoryPropertyFlags memProps)
		: device{ device }
	{
		create(memProps, info);
	}

	Image::~Image() 
	{
		destroyView();
		if (imageMemory != VK_NULL_HANDLE) 
		{ 
			destroyImage();
			vkFreeMemory(device.device(), imageMemory, nullptr); 
		}
		if (sampler != VK_NULL_HANDLE) 
		{ 
			vkDestroySampler(device.device(), sampler, nullptr); 
		}
	}

	void Image::destroyView() 
	{
		if (imageView == VK_NULL_HANDLE) { return; }
		vkDestroyImageView(device.device(), imageView, nullptr);
		imageView = VK_NULL_HANDLE;
	}

	void Image::destroyImage()
	{
		if (image == VK_NULL_HANDLE) { return; }
		vkDestroyImage(device.device(), image, nullptr);
		image = VK_NULL_HANDLE;
	}

	void Image::loadFromDisk(const std::string& path)
	{
		// import (see Vulkan Tutorial - Texture mapping)
		int width, height, channels;
		stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		VkDeviceSize imageSize = width * height * (uint32_t)4;
		if (!pixels) { throw std::runtime_error("failed to load image " + path); }

		// temporary image buffer
		GBuffer stagingBuffer
		{
			device, imageSize, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		// map buffer to host and copy to it
		stagingBuffer.map(imageSize);
		memcpy(stagingBuffer.getMappedMemory(), pixels, static_cast<size_t>(imageSize)); 
		stagingBuffer.unmap();

		stbi_image_free(pixels); // free importer memory

		/*	allocate and prep the image for write, device local memory is fast but does not allow host access */
		VkImageCreateInfo info = makeImageCreateInfo(width, height); // using defaults
		create(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, info);

		// transfer data from buffer to image
		copyBufferToImage(stagingBuffer, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);
	}

	VkImageCreateInfo Image::makeImageCreateInfo(uint32_t width, uint32_t height)
	{
		VkImageCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ci.imageType = VK_IMAGE_TYPE_2D; // regular flat image texture
		ci.extent.width = width;
		ci.extent.height = height;
		ci.extent.depth = 1;
		ci.mipLevels = 1;
		ci.arrayLayers = 1;
		ci.format = VK_FORMAT_R8G8B8A8_SRGB; // format must be supported by GPU
		ci.tiling = VK_IMAGE_TILING_OPTIMAL;
		ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // shader sample-able
		ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ci.samples = VK_SAMPLE_COUNT_1_BIT;
		return ci;
	}

	void Image::create(VkMemoryPropertyFlags memProps, VkImageCreateInfo info)
	{
		// performs the memory allocation and creation of the underlying VkImage
		if (vkCreateImage(device.device(), &info, nullptr, &image) != VK_SUCCESS)
		{ throw std::runtime_error("failed to create image"); }

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device.device(), image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, memProps);

		if (vkAllocateMemory(device.device(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{ throw std::runtime_error("failed to allocate memory for image"); }

		if (vkBindImageMemory(device.device(), image, imageMemory, 0) != VK_SUCCESS)
		{ throw std::runtime_error("failed to bind memory for image"); }
	}

	void Image::transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else { throw std::invalid_argument("unsupported image layout transition"); }
		
		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
							nullptr, 0, nullptr, 1, &barrier);

		device.endSingleTimeCommands(commandBuffer);
	}

	void Image::copyBufferToImage(const GBuffer& buffer, uint32_t width, uint32_t height, uint32_t layerCount)
	{
		// vkCmdCopyBufferToImage requires the right image layout, that is handled here
		transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount; // default = 1
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer.getBuffer(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		device.endSingleTimeCommands(commandBuffer);
		// transition (again) to a more useful format
		transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void Image::createView(VkImageView& view, VkFormat format, VkImageAspectFlags aspect, VkImageViewType viewType)
	{
		assert(image != VK_NULL_HANDLE && "failed to create image view, image was uninitialized");
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = image;
		info.format = format;
		info.viewType = viewType;
		info.subresourceRange.aspectMask = aspect;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;
		if (vkCreateImageView(device.device(), &info, nullptr, &view) != VK_SUCCESS) 
		{ throw std::runtime_error("failed to create image view"); }
	}
	
	void Image::updateView(VkFormat format, VkImageAspectFlags aspect, VkImageViewType viewType)
	{
		destroyView();
		createView(imageView, format, aspect, viewType);
	}

	void Image::createSampler(VkSampler& samplerHandleOut, EngineDevice& device, const float& anisotropy)
	{
		VkSamplerCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.magFilter = VK_FILTER_LINEAR;
		info.minFilter = VK_FILTER_LINEAR;

		info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		info.unnormalizedCoordinates = VK_FALSE;

		info.anisotropyEnable = anisotropy > 0.f ? VK_TRUE : VK_FALSE;
		info.maxAnisotropy = anisotropy;

		info.compareEnable = VK_FALSE;
		info.compareOp = VK_COMPARE_OP_ALWAYS;

		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		info.mipLodBias = 0.0f;
		info.minLod = 0.0f;
		info.maxLod = 0.0f;

		if (vkCreateSampler(device.device(), &info, nullptr, &samplerHandleOut) != VK_SUCCESS)
		{ throw std::runtime_error("failed to create texture sampler"); }
	}

    void Image::setDebugName(const std::string& name)
    {
		if (not device.enableValidationLayers)
			return;

		debugName = name;
		PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT = 
				(PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(device.getVulkanInstance(), "vkSetDebugUtilsObjectNameEXT");
		const VkDebugUtilsObjectNameInfoEXT imageNameInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = NULL,
			.objectType = VK_OBJECT_TYPE_IMAGE,
			.objectHandle = (uint64_t)image,
			.pObjectName = debugName.c_str(),
		};
		pfnSetDebugUtilsObjectNameEXT(device.device(), &imageNameInfo);
    }

}