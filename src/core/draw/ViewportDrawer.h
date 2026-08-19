// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once
#include "core/draw/DrawBase.h"
#include "core/types/CommonTypes.h"

#include "core/types/vk.h"

#include <array>
#include <memory>
#include <vector>

namespace EngineCore
{
	class EngineDevice;
	class DescriptorSet;
	class Material;

	class ViewportDrawer : public DrawBase
	{
	public:
		ViewportDrawer(EngineDevice& device, const DrawContext& d);
		~ViewportDrawer();

		virtual void render(const FrameContext& f) override;

		void setPositionAndSize(Vec2 position, Vec2 size);

	private:
		std::unique_ptr<DescriptorSet> attachmentSet; // attachment image bindings, same number of internal sets as swapchain images
		std::unique_ptr<DescriptorSet> samplerSet; // treated as any other descriptor set (using frames in flight number)
		VkSampler attachmentSampler;
		std::unique_ptr<Material> viewportMaterial;
		Vec2 viewportPosition;
		Vec2 viewportSize;

		void bindDescriptorSets(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, uint32_t frameIndex, uint32_t swapImageIndex);
		VkSampler createSampler();
	};

}