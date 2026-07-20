#pragma once
#include "core/types/vk.h"
#include "core/types/CommonTypes.h"

#include <memory>
#include <string>
#include <vector>

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

		void renderSky(VkCommandBuffer commandBuffer, VkDescriptorSet sceneGlobalDescriptorSet, Vec observerPosition);

		float skyMeshScale = 1000.f * 10.f;

	private:
		std::unique_ptr<Nodes::MeshNode> skyMesh;
		
	};

}
