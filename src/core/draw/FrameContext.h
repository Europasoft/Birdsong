#pragma once

#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/engine/Camera.h"
#include "core/gpu/Material.h"
#include "core/types/CommonTypes.h"

#include "core/types/vk.h"

namespace EngineCore
{
	class Renderer;
	struct RenderingFormats;
	class ViewportDrawer;

	struct ViewportState
	{
		Vec2 extent; // either viewport extent, or window/swapchain extent (in final renderpass)
		Vec2 position; // zero or position of the virtual viewport UI element (in pixels)

		// C++20: const operator== allows automatic operator!= generation
		bool operator==(const ViewportState& b) const noexcept { return extent == b.extent && position == b.position; }
	};

	struct FrameContext
	{
		VkCommandBuffer commandBuffer;
		double delta = 0;
		uint32_t bufferIndex = 0;
		WorldSystem::World* world;
		WorldSystem::Scene* scene;
		Camera* camera;
		ViewportState viewport;
		Vec264 mousePosition;
		bool leftClick, rightClick;
	};

	struct DrawContext
	{
		Renderer* renderer;
		WorldSystem::World* world;
		RenderingFormats basePassFormats;
		RenderingFormats fxPassFormats;
		RenderingFormats postFxPassFormats;
		VkSampleCountFlagBits samples;
	};

}