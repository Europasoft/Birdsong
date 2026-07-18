#pragma once
#include "core/engine/Window.h"
#include "core/gpu/Device.h"
#include "core/render/Renderer.h"
#include "core/engine/Camera.h"
#include "core/draw/DrawIncludes.h"
#include "core/world/World.h"

#include <memory>
#include <vector>
#include <chrono> // timing
#include <algorithm> // min()

#include "core/types/CommonTypes.h"
#include "core/gpu/Descriptors.h"
#include "core/engine/EngineSettings.h"
#include "core/engine/EngineClock.h"

class SharedMaterialsPool;

namespace EngineCore
{
	class Image;
	class GameLoader;

	// base class for an object representing the entire engine
	class EngineApplication
	{
	public:
		EngineApplication();
		~EngineApplication();
		EngineApplication(const EngineApplication&) = delete;
		EngineApplication& operator=(const EngineApplication&) = delete;

		// hardcoded window size in pixels
		static constexpr int WIDTH = 1920;
		static constexpr int HEIGHT = 1080;

		// begins the main window event loop
		void startExecution();

		const EngineRenderSettings& getRenderSettings() const { return renderSettings; }
		Renderer& getRenderer() { return renderer; }

	private:
		void setupDrawers();
		void setupDefaultInputs();

		void onSwapchainCreated();
		void render();
		void moveCamera(Camera& camera);

		glm::vec3 unproject(glm::vec3 point);

		EngineRenderSettings renderSettings{};

		// engine application window (creates a window using GLFW) 
		EngineWindow window{ WIDTH, HEIGHT, "Vulkan Window" };

		// render device (instantiates vulkan)
		EngineDevice device{ window };
		
		// the renderer manages the swapchain, renderpasses, and the vulkan command buffers
		Renderer renderer{ window, device, renderSettings };

		EngineClock engineClock{};

		std::unique_ptr<MeshDrawer> meshDrawer;
		std::unique_ptr<SkyDrawer> skyDrawer;
		std::unique_ptr<FxDrawer> fxDrawer;
		std::unique_ptr<InterfaceDrawer> uiDrawer;
		std::unique_ptr<DebugDrawer> debugDrawer;

		std::unique_ptr<GameLoader> gameLoader;
		

		Vec mouseMoveObjectOriginalLocation;
		bool movingObjectWithCursor = true;

		WorldSystem::World world{ device, *this };

	};

}