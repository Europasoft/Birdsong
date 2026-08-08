#pragma once
#include "core/draw/DrawBase.h"
#include "core/types/vk.h"
#include <array>
#include <memory>
#include <vector>

namespace WorldSystem
{
	class EngineNodeData;
}

namespace EngineCore
{
	class EngineDevice;
	class Renderer;
	class DescriptorSet;
	class Material;
	struct RenderingFormats;
	
	class FxDrawer : public DrawBase
	{
	public:
		FxDrawer(EngineDevice& device, const DrawContext& d);
		~FxDrawer();

		virtual void render(const FrameContext& f) override;

	private:
		std::unique_ptr<DescriptorSet> uboSet; // additional data, treated as any other descriptor set (using frames in flight number)
		std::unique_ptr<DescriptorSet> attachmentSet; // attachment image bindings, same number of internal sets as swapchain images
		VkSampler attachmentSampler;
		std::unique_ptr<WorldSystem::EngineNodeData> enode;
		std::unique_ptr<Material> fullscreenMaterial;

		void bindDescriptorSets(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, uint32_t frameIndex, uint32_t swapImageIndex);
		VkSampler createSampler();
	};

}
