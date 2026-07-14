#pragma once
#include <vulkan/vulkan.h>
#include "Core/GPU/Device.h"
#include "Core/Types/CommonTypes.h"

#include <stdexcept>

namespace EngineCore 
{

	struct SampleCountSetting
	{
		// return in vulkan-usable format
		operator VkSampleCountFlagBits() const { return samples; }
		// sets the closest multisampling level supported by the device, must be power of two
		void set(uint32_t s, EngineDevice& device)
		{
			if (s % 2 != 0) { throw std::runtime_error("tried to set invalid multisampling level"); }
			// enforce a minimum of 2 samples, since most GPUs should support this
			if (s <= VK_SAMPLE_COUNT_2_BIT) { samples = VK_SAMPLE_COUNT_2_BIT; } 
			else if ((VkSampleCountFlagBits)s >= device.getMaxSampleCount()) { samples = device.getMaxSampleCount(); }
			else
			{	/* we could use uint32 bits directly, but this ugly mess makes sure we're safe
				 in the unlikely event that the vulkan spec changes in some weird way */
				switch (s) 
				{
				case 4: samples = VK_SAMPLE_COUNT_4_BIT;
					break;
				case 8: samples = VK_SAMPLE_COUNT_8_BIT;
					break;
				case 16: samples = VK_SAMPLE_COUNT_16_BIT;
					break;
				case 32: samples = VK_SAMPLE_COUNT_32_BIT;
					break;
				case 64: samples = VK_SAMPLE_COUNT_64_BIT;
					break;
				default: samples = VK_SAMPLE_COUNT_2_BIT; // edge case
				}
			}
		}
	private:
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_8_BIT; // default
	};

	struct CascadedShadowSettings
	{
		// resolution (width/height) shared by every cascade image
		VkExtent2D extent = { 4096, 4096 };
		// depth format used for the cascade attachment images
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
		// number of cascades rendered each frame (must be <= MAX_SHADOW_CASCADES)
		uint32_t cascadeCount = 4;
		// splits between linear (0.0) and logarithmic (1.0) distribution of cascade ranges
		float splitLambda = 0.92f;
	};

	struct EngineRenderSettings
	{
		SampleCountSetting sampleCountMSAA;
		CascadedShadowSettings cascadedShadows{};
	};

}




