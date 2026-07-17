#pragma once
#include "core/types/vk.h"

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Nodes
{
	class MeshNode;
}

namespace EngineCore
{
	class EngineDevice;
	class DescriptorSet;
	struct RenderingFormats;

	class SkyDrawer 
	{
	public:
		SkyDrawer(EngineDevice& device, DescriptorSet& defaultSet, const RenderingFormats& formats, VkSampleCountFlagBits samples);
		~SkyDrawer();

		void renderSky(VkCommandBuffer commandBuffer, VkDescriptorSet sceneGlobalDescriptorSet, 
						const glm::vec3& observerPosition);

		float skyMeshScale = 1000.f * 10.f;

	private:
		std::unique_ptr<Nodes::MeshNode> skyMesh;
		
	};

}
