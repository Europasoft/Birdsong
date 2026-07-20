#pragma once
#include "core/types/CommonTypes.h"
#include "core/gpu/Descriptors.h"
#include "core/engine/EngineSettings.h"
#include "core/engine/EngineClock.h"

#include <memory>
#include <vector>
#include <chrono> // timing
#include <algorithm> // min()

namespace WorldSystem
{
	class World;
}

namespace EngineCore
{
	class EngineWindow;
	class EngineDevice;
	class Renderer;
	class MeshDrawer;
	class SkyDrawer;
	class FxDrawer;
	class InterfaceDrawer;
	class DebugDrawer;
	class GameLoader;
	class Camera;

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
		Renderer& getRenderer();

	private:
		friend class GameLoader;
		friend class IEngineImpl;

		void setupDrawers();
		void setupDefaultInputs();

		void onSwapchainCreated();

		void mainLoop();
		struct FrameContext;
		void render(const FrameContext& frame);
		void moveCamera(Camera& camera);

		EngineRenderSettings renderSettings{};

		// engine application window (creates a window using GLFW) 
		std::unique_ptr<EngineWindow> window;

		// render device (instantiates vulkan)
		std::unique_ptr<EngineDevice> device;
		
		// the renderer manages the swapchain, renderpasses, and the vulkan command buffers
		std::unique_ptr<Renderer> renderer;

		EngineClock engineClock{};

		std::unique_ptr<MeshDrawer> meshDrawer;
		std::unique_ptr<SkyDrawer> skyDrawer;
		std::unique_ptr<FxDrawer> fxDrawer;
		std::unique_ptr<InterfaceDrawer> uiDrawer;
		std::unique_ptr<DebugDrawer> debugDrawer;

		std::unique_ptr<GameLoader> gameLoader;
		std::unique_ptr<WorldSystem::World> world;
		

		Vec mouseMoveObjectOriginalLocation;
		bool movingObjectWithCursor = true;

		

	};

}