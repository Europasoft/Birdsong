#pragma once
#include "core/types/vk.h"
#include <array>
#include <memory>
#include <vector>

namespace Nodes
{
	class MeshNode;
}

namespace EngineCore
{
	class EngineDevice;
	class Renderer;
	class DescriptorSet;
	class Material;
	struct RenderingFormats;
	
	class FxDrawer
	{
	public:
		FxDrawer(EngineDevice& device, DescriptorSet& defaultSet, const RenderingFormats& formats,
				const std::vector<VkImageView>& inputImageViews, const std::vector<VkImageView>& inputDepthImageViews);
		~FxDrawer();

		void render(VkCommandBuffer cmdBuffer, Renderer& renderer);

	private:
		EngineDevice& device;
		
		DescriptorSet& defaultSet;
		std::unique_ptr<DescriptorSet> uboSet; // additional data, treated as any other descriptor set (using frames in flight number)
		std::unique_ptr<DescriptorSet> attachmentSet; // attachment image bindings, same number of internal sets as swapchain images
		std::unique_ptr<Nodes::MeshNode> mesh;
		std::unique_ptr<Material> fullscreenMaterial;

		void bindDescriptorSets(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, uint32_t frameIndex, uint32_t swapImageIndex);
	};

}
