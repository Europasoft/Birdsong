#pragma once
#include "Core/Window.h"
#include "Core/GPU/Device.h"
#include "Core/Renderer.h"
#include "Core/Camera.h"
#include "Core/Draw/MeshDrawer.h"
#include "Core/Draw/SkyDrawer.h"
#include "Core/Draw/FxDrawer.h"
#include "Core/Draw/InterfaceDrawer.h"

#include "Core/Physics/PhysicsScene.h"

#include <memory>
#include <vector>
#include <chrono> // timing
#include <algorithm> // min()

#include "Core/Types/CommonTypes.h"
#include "Core/GPU/Descriptors.h"
#include "Core/EngineSettings.h"

class SharedMaterialsPool;

namespace EngineCore
{
	class EngineClock
	{
		using clock = std::chrono::steady_clock;
		using timePoint = clock::time_point;
		const double deltaMax = 0.2;
	public:
		EngineClock() : start{ clock::now() } {};
		void measureFrameDelta(const uint32_t& currentframeIndex) 
		{
			if (currentframeIndex != lastFrameIndex) /* new frame started? */
			{
				if (lastFrameIndex != 959) /* update delta (except on very first frame) */
				{
					std::chrono::duration<double, std::milli> ms = clock::now() - frameDeltaStart;
					frameDelta = std::min(deltaMax, (ms.count() / 1000.0)); 
				}
				// reset timer
				frameDeltaStart = clock::now();
				lastFrameIndex = currentframeIndex;
			}
		}
		const double& getDelta() const { return frameDelta; }
		uint32_t getFps() const { return (uint32_t) 1 / frameDelta; }
		double getElapsed() const
		{
			std::chrono::duration<double, std::milli> ms = clock::now() - start;
			return ms.count() / 1000.0;
		}
	private:
		timePoint start;
		timePoint frameDeltaStart;
		double frameDelta = 0.01;
		uint32_t lastFrameIndex = 959;
	};

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
		static constexpr int WIDTH = 1920; //1100;
		static constexpr int HEIGHT = 1080; //720;

		EngineRenderSettings renderSettings{};

		// begins the main window event loop
		void startExecution();

		// TODO: move this function somewhere else
		// also remove temporaries, search for "FakeScaleTest082"
		//void simulateDistanceByScale(const StaticMesh& mesh, const Transform& cameraTransform);
		Transform simDistOffsets{};
		//static double ddist(const Vector3D<double>& a, const Vector3D<double>& b);
		//static Vector3D<double> ddir(const Vector3D<double>& a, const Vector3D<double>& b);

		void applyWorldOriginOffset(Transform& cameraTransform);

	private:
		void loadDemoMeshes();
		void setupDefaultInputs();
		void setupDescriptors();
		void applyDemoMaterials();
		void setupDrawers();
		void onSwapchainCreated();
		void render();
		void updateDescriptors(uint32_t frameIndex);
		void moveCamera();
		glm::mat4 getProjectionViewMatrix(bool inverse = false);

		void testMoveObjectWithMouse();
		glm::vec3 getMouseMove3DLocationTest_legacy(float planeDistance, float dist2 = 1.f);
		glm::vec3 unproject(glm::vec3 point);

		// engine application window (creates a window using GLFW) 
		EngineWindow window{ WIDTH, HEIGHT, "Vulkan Window" };

		// render device (instantiates vulkan)
		EngineDevice device{ window };
		
		// the renderer manages the swapchain, renderpasses, and the vulkan command buffers
		Renderer renderer{ window, device, renderSettings };

		EngineClock engineClock{};

		DescriptorSet dset{ device }; // default global descriptor set

		std::unique_ptr<DescriptorPool> globalDescriptorPool{};
		std::vector<std::unique_ptr<Primitive>> loadedMeshes;

		Camera camera;
		std::unique_ptr<Image> spaceTexture;
		std::unique_ptr<Image> marsTexture;
		std::unique_ptr<MeshDrawer> meshDrawer;
		std::unique_ptr<SkyDrawer> skyDrawer;
		std::unique_ptr<FxDrawer> fxDrawer;
		std::unique_ptr<InterfaceDrawer> uiDrawer;

		// TODO: this is strictly temporary
		glm::vec3 lightPos{ -20.f, 100.f, 45.f };

		Vec mouseMoveObjectOriginalLocation;
		bool movingObjectWithCursor = true;

	};

}