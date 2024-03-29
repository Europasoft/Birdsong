#pragma once
#include "Core/vk.h"

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace EngineCore
{
	class EngineDevice;
	class Primitive;

	class SkyDrawer 
	{
	public:
		SkyDrawer(const std::vector<VkDescriptorSetLayout>& setLayouts,
						EngineDevice& device, VkSampleCountFlagBits samples, VkRenderPass renderpass);

		void renderSky(VkCommandBuffer commandBuffer, VkDescriptorSet sceneGlobalDescriptorSet, 
						const glm::vec3& observerPosition);

	private:
		std::unique_ptr<Primitive> skyMesh;
	};

}
