#pragma once
#include "Core/Window.h"
#include "Core/GPU/Device.h"
#include "Core/Render/Renderer.h"
#include "Core/Camera.h"
#include "Core/Draw/DrawIncludes.h"
#include "Core/WorldSystem/World.h"
#include "Core/Physics/PhysicsScene.h"

#include <memory>
#include <vector>
#include <chrono> // timing
#include <algorithm> // min()

#include "Core/Types/CommonTypes.h"
#include "Core/GPU/Descriptors.h"
#include "Core/EngineSettings.h"
#include "Core/EngineClock.h"

class SharedMaterialsPool;

namespace EngineCore
{
	class Primitive;
	class Image;

	// base class for an object representing the entire engine
	class EngineApplication
	{
	public:
		EngineApplication() = default;
		~EngineApplication() = default;
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


		std::vector<std::unique_ptr<Primitive>> loadedMeshes;// moved to world/sector system

		std::unique_ptr<MeshDrawer> meshDrawer;
		std::unique_ptr<SkyDrawer> skyDrawer;
		std::unique_ptr<FxDrawer> fxDrawer;
		std::unique_ptr<InterfaceDrawer> uiDrawer;
		std::unique_ptr<DebugDrawer> debugDrawer;

		
		

		Vec mouseMoveObjectOriginalLocation;
		bool movingObjectWithCursor = true;

		WorldSystem::World world{ device, *this };

	};

}