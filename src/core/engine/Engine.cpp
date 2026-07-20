#include "core/engine/Engine.h"

#include "core/engine/Window.h"
#include "core/gpu/Device.h"
#include "core/render/Renderer.h"
#include "core/draw/DrawIncludes.h"
#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/engine/Camera.h"
#include "core/gpu/Material.h"
#include "core/gpu/Buffer.h"
#include "core/gpu/Image.h"
#include "core/engine/interop/GameLoader.h"

#include <stdexcept>
#include <array>
#include <iostream>
#include <string>

#include <GLFW/glfw3.h> // GL Framework (GLFW) used to create an engine window
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace EngineCore
{
	EngineApplication::EngineApplication() {}
	EngineApplication::~EngineApplication() {}

	void EngineApplication::startExecution()
	{
		std::cout << "Engine working directory: '" << std::filesystem::current_path().string() << "'\n";
		window = std::make_unique<EngineWindow>(WIDTH, HEIGHT, "Vulkan Window");
		device = std::make_unique<EngineDevice>(*window);
		renderer = std::make_unique<Renderer>(*window, *device, renderSettings);
		world = std::make_unique<WorldSystem::World>(*device, *this);
		gameLoader = std::make_unique<GameLoader>(this);
		gameLoader->loadDll("Game");

		renderer->swapchainCreatedCallback = std::bind(&EngineApplication::onSwapchainCreated, this);
		world->loadDemoScene();
		setupDrawers();
		setupDefaultInputs();

		// keep running until application is closed
		mainLoop();

		// window pending close, wait for GPU
		vkDeviceWaitIdle(device->device());
	}

	struct EngineApplication::FrameContext
	{
		VkCommandBuffer commandBuffer;
		double delta;
		uint32_t bufferIndex;
		WorldSystem::Scene* scene;
		Camera* camera;
	};

	void EngineApplication::mainLoop()
	{
		// window event loop
		EngineApplication::FrameContext f = {};
		while (!window->getCloseWindow())
		{
			window->input.resetInputValues(); // reset input values
			window->input.updateBoundInputs(); // get new input states
			window->pollEvents(); // process events in window queue

			f.scene = &world->getScene();
			f.camera = &f.scene->getCurrentCamera();

			// engine tick updates
			moveCamera(*f.camera);
			f.camera->testValue += window->input.getAxisValue(4) * static_cast<float>(f.delta) * 2.f;
			gameLoader->tick(f.delta);
			f.scene->sectorUpdate(*f.camera);

			// render frame
			f.commandBuffer = renderer->beginFrame();
			if (f.commandBuffer)
			{
				f.bufferIndex = renderer->getFrameIndex();
				f.delta = engineClock.measureFrameDelta(f.bufferIndex);
				render(f);
			}
		}
	}

	void EngineApplication::setupDrawers() 
	{
		const auto& baseFormats =	renderer->getBasePassFormats();
		const auto& fxFormats =		renderer->getFxPassFormats();

		auto& sceneGlobalDescriptorSet = world->getScene().getSceneGlobalDescriptorSet();
		meshDrawer = std::make_unique<MeshDrawer>(*device);
		skyDrawer = std::make_unique<SkyDrawer>(*device, sceneGlobalDescriptorSet, baseFormats, renderSettings.sampleCountMSAA);
		fxDrawer = std::make_unique<FxDrawer>(*device, sceneGlobalDescriptorSet, fxFormats, renderer->getFxPassInputImageViews(), renderer->getFxPassInputDepthImageViews());
		uiDrawer = std::make_unique<InterfaceDrawer>(*device, baseFormats, renderSettings.sampleCountMSAA);
		debugDrawer = std::make_unique<DebugDrawer>(*device, sceneGlobalDescriptorSet, baseFormats, renderSettings.sampleCountMSAA);
	}

	void EngineApplication::setupDefaultInputs()
	{
		InputSystem& inputSys = window->input;

		inputSys.captureMouseCursor(true);

		// add binding for forwards (and backwards) movement
		uint32_t fwdAxisIndex = inputSys.addBinding(KeyBinding(GLFW_KEY_W, 1.f), "kbForwardAxis");
		inputSys.addBinding(KeyBinding(GLFW_KEY_S, -1.f), fwdAxisIndex);
		// right/left
		uint32_t rightAxisIndex = inputSys.addBinding(KeyBinding(GLFW_KEY_D, 1.f), "kbRightAxis");
		inputSys.addBinding(KeyBinding(GLFW_KEY_A, -1.f), rightAxisIndex);
		// up/down
		uint32_t upAxisIndex = inputSys.addBinding(KeyBinding(GLFW_KEY_R, 1.f), "kbUpAxis");
		inputSys.addBinding(KeyBinding(GLFW_KEY_F, -1.f), upAxisIndex);
		// move faster
		inputSys.addBinding(KeyBinding(GLFW_KEY_LEFT_SHIFT, 1.f), "kbFasterAxis");
		// test value, used for something
		uint32_t testAxisIndex = inputSys.addBinding(KeyBinding(GLFW_KEY_UP, 1.f), "kbUpAxis");
		inputSys.addBinding(KeyBinding(GLFW_KEY_DOWN, -1.f), testAxisIndex);
		// g, x, y, and z -buttons
		inputSys.addBinding(KeyBinding(GLFW_KEY_G, 1.f), "kbG-keyAxis");
		inputSys.addBinding(KeyBinding(GLFW_KEY_X, 1.f), "kbX-keyAxis");
		inputSys.addBinding(KeyBinding(GLFW_KEY_Y, 1.f), "kbY-keyAxis");
		inputSys.addBinding(KeyBinding(GLFW_KEY_Z, 1.f), "kbZ-keyAxis");
	}

	void EngineApplication::onSwapchainCreated()
	{
		// fxDrawer uses swapchain image count, since it samples from the swapchain attachments, so it must be recreated together with the swapchain
		setupDrawers();
	}

	void EngineApplication::render(const FrameContext& f)
	{
		f.scene->updateDescriptors(f.bufferIndex, f.delta);

		debugDrawer->removeDebugBoxes();
		debugDrawer->addDebugBox(Vec(static_cast<float>(f.scene->getSectorSize())), Vec(0.f), Vec(0.f, 0.f, .8f), 0.5f);
		
		// RENDER BASE PASS
		renderer->beginRenderingBase(f.commandBuffer); 

		// render sky sphere
		skyDrawer->renderSky(f.commandBuffer, f.scene->getSceneGlobalDescriptorSet().getDescriptorSet(f.bufferIndex), f.camera->transform.translation);

		// render meshes
		meshDrawer->renderMeshes(f.commandBuffer, *world, f.delta, engineClock.getElapsed(), f.bufferIndex,
			f.scene->getSceneGlobalDescriptorSet().getDescriptorSet(f.bufferIndex), f.camera->getProjectionViewMatrix());

		debugDrawer->render(f.commandBuffer, *renderer);

		renderer->endRendering(f.commandBuffer);

		// RENDER FX PASS
		fxDrawer->render(f.commandBuffer, *renderer);

		// submit command buffer
		renderer->endFrame(); 
		f.camera->setAspectRatio(renderer->getSwapchainAspectRatio());
	}

	void EngineApplication::moveCamera(Camera& camera)
	{
		auto mf = window->input.getAxisValue(0);
		auto mr = window->input.getAxisValue(1);
		auto mu = window->input.getAxisValue(2);
		auto xs = window->input.getAxisValue(3) > 0 ? true : false;
		auto lookInput = window->input.getMouseDelta();
		camera.moveInPlaneXY(lookInput, mf, mr, mu, xs, static_cast<float>(engineClock.getDelta()));
	}

	//glm::vec3 EngineApplication::unproject(glm::vec3 point)
	//{
	//	const auto pvm = world->getScene().getCurrentCamera().getProjectionViewMatrix(true);
	//	glm::vec4 v = pvm * glm::vec4(point.x, point.y, point.z, 1);
	//	return glm::vec3(v.x, v.y, v.z) / v.w;
	//}

	Renderer& EngineApplication::getRenderer()
	{
		return *renderer;
	}

} 