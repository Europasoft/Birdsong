#pragma once

#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/engine/Camera.h"
#include "core/gpu/Material.h"

#include "core/types/vk.h"

namespace EngineCore
{
	class Renderer;
	struct RenderingFormats;
	class ViewportDrawer;

	struct FrameContext
	{
		VkCommandBuffer commandBuffer;
		double delta = 0;
		uint32_t bufferIndex = 0;
		WorldSystem::World* world;
		WorldSystem::Scene* scene;
		Camera* camera;
		VkExtent2D viewportExtent;
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
		ViewportDrawer* viewportDrawer;
	};
}