#pragma once
#pragma warning(push, 0) // warning-ignore hack only works in header
#include <vulkan/vulkan.h>
#pragma warning(pop)
#include <string>

namespace EngineCore
{
	class EngineDevice;
	class GBuffer;

	/* Image is an abstraction for an image or texture in video memory (VkImage), as the name implies */
	class Image
	{
	public:
		Image(EngineDevice& device, VkImage image);
		Image(EngineDevice& device, const std::string& path);
		Image(EngineDevice& device, VkImageCreateInfo info, VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		~Image();

		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;

		VkImage getImage() const { return image; }
		VkImageView getView() const { return imageView; }
		VkDeviceMemory getMemory() { return imageMemory; }

		void updateView(VkFormat format, VkImageAspectFlags aspect, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
		// returns a new image view using the current image, does not update the default view
		void createView(VkImageView& view, VkFormat format, VkImageAspectFlags aspect, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
		
		static VkImageCreateInfo makeImageCreateInfo(uint32_t width, uint32_t height);
		static void createSampler(VkSampler& samplerHandleOut, EngineDevice& device, const float& anisotropy = 0.f);

		void setDebugName(const std::string& name);

		VkSampler sampler = VK_NULL_HANDLE; // samplers are not connected to specific images
	private:
		EngineDevice& device;
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory imageMemory = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE; // default image view
		std::string debugName;

		void create(VkMemoryPropertyFlags memProps, VkImageCreateInfo info);
		void loadFromDisk(const std::string& path);
		void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void copyBufferToImage(const GBuffer& buffer, uint32_t width, uint32_t height, uint32_t layerCount);
		void destroyView();
		void destroyImage();

	};
}

