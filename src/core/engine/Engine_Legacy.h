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
		static constexpr int WIDTH = 1920; //1100;
		static constexpr int HEIGHT = 1080; //720;


		// begins the main window event loop
		void startExecution();

		// TODO: move this function somewhere else
		// also remove temporaries, search for "FakeScaleTest082"
		//void simulateDistanceByScale(const StaticMesh& mesh, const Transform& cameraTransform);
		Transform simDistOffsets{};
		//static double ddist(const Vector3D<double>& a, const Vector3D<double>& b);
		//static Vector3D<double> ddir(const Vector3D<double>& a, const Vector3D<double>& b);

		//void applyWorldOriginOffset(Transform& cameraTransform);

		VkDescriptorSetLayout getGlobalDescriptorLayout() const { return dset.getLayout(); }
		const EngineRenderSettings& getRenderSettings() const { return renderSettings; }
		Renderer& getRenderer() { return renderer; }

	private:
		void loadDemoScene();
		void setupDefaultInputs();
		void setupDescriptors();
		void applyDemoMaterials();//moved to world/sector system
		void setupDrawers();
		void onSwapchainCreated();
		void render();
		void updateDescriptors(uint32_t frameIndex);
		void moveCamera();
		glm::mat4 getProjectionViewMatrix(bool inverse = false);

		void testMoveObjectWithMouse();
		glm::vec3 getMouseMove3DLocationTest_legacy(float planeDistance, float dist2 = 1.f);
		glm::vec3 unproject(glm::vec3 point);

		EngineRenderSettings renderSettings{};

		// engine application window (creates a window using GLFW) 
		EngineWindow window{ WIDTH, HEIGHT, "Vulkan Window" };

		// render device (instantiates vulkan)
		EngineDevice device{ window };
		
		// the renderer manages the swapchain, renderpasses, and the vulkan command buffers
		Renderer renderer{ window, device, renderSettings };

		EngineClock engineClock{};

		// default global descriptor set
		DescriptorSet dset{ device }; 

		std::unique_ptr<DescriptorPool> globalDescriptorPool{};
		std::vector<std::unique_ptr<Primitive>> loadedMeshes;// moved to world/sector system

		Camera camera;
		std::unique_ptr<Image> spaceTexture;
		std::unique_ptr<Image> marsTexture;

		std::unique_ptr<MeshDrawer> meshDrawer;
		std::unique_ptr<SkyDrawer> skyDrawer;
		std::unique_ptr<FxDrawer> fxDrawer;
		std::unique_ptr<InterfaceDrawer> uiDrawer;
		std::unique_ptr<DebugDrawer> debugDrawer;

		// TODO: this is strictly temporary
		glm::vec3 lightPos{ -20.f, 100.f, 45.f };

		Vec mouseMoveObjectOriginalLocation;
		bool movingObjectWithCursor = true;

		WorldSystem::World world{ device, *this };

	};

}