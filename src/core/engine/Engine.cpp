#include "core/engine/Engine.h"
#include "core/draw/FrameContext.h"
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
#include "core/draw/planets/PlanetDrawer.h"
#include "core/ui/Fonts.h"
#include "editor/Editor.h"

#include <stdexcept>
#include <array>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include <GLFW/glfw3.h> // GL Framework (GLFW) used to create an engine window
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace EngineCore
{
	EngineApplication::EngineApplication() {}

	EngineApplication::~EngineApplication()
	{
		gameLoader.reset(); // makes sure the game is unloaded before engine state teardown
	}

	void EngineApplication::startExecution()
	{
		std::cout << "Engine working directory: '" << std::filesystem::current_path().string() << "'\n";
		window = std::make_unique<EngineWindow>(WIDTH, HEIGHT, "Vulkan Window");
		device = std::make_unique<EngineDevice>(*window);
		renderer = std::make_unique<Renderer>(*window, *device, renderSettings);
		world = std::make_unique<WorldSystem::World>(*device, *this);
		gameLoader = std::make_unique<GameLoader>(this);
		editor = std::make_unique<Editor::Editor>();
		try
		{
			gameLoader->loadDll("Game");
		}
		catch (...) { std::cout << "Failed to load Game DLL"; }

		renderer->swapchainCreatedCallback = std::bind(&EngineApplication::onSwapchainCreated, this);

		setupDrawers();
		setupDefaultInputs();

		// keep running until application is closed
		mainLoop();

		// window pending close, wait for GPU
		device->waitIdle();
	}

	void EngineApplication::mainLoop()
	{
		// window event loop
		FrameContext f = {};
		while (not (window->getCloseWindow() || exitApplication))
		{
			inputTick(f.delta);

			f.world = world.get();
			f.scene = &f.world->getScene();
			f.camera = &f.scene->getCurrentCamera();
			f.delta = engineClock.measureFrameDelta(f.bufferIndex);
			f.viewportExtent = renderer->getViewportExtent();
			f.mousePosition = window->input.getMousePosition();
			f.leftClick = window->input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
			f.rightClick = window->input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);

			// engine tick updates
			moveCamera(*f.camera);
			gameLoader->gameTick(f.delta);
			f.scene->updateNodes();
			f.scene->sectorUpdate(*f.camera);
			f.scene->physicsTick();

			// render frame
			f.commandBuffer = renderer->beginFrame();
			if (f.commandBuffer)
			{
				f.bufferIndex = renderer->getFrameIndex();
				render(f);
			}
			f.scene->gamePostPhysicsUpdate();
		}
	}

	void EngineApplication::setupDrawers() 
	{
		drawContext = std::make_unique<DrawContext>();
		drawContext->world = world.get();
		drawContext->renderer = renderer.get();
		drawContext->basePassFormats = renderer->getBasePassFormats();
		drawContext->fxPassFormats = renderer->getFxPassFormats();
		drawContext->postFxPassFormats = renderer->getPostFxPassFormats();
		drawContext->samples = renderSettings.sampleCountMSAA;

		auto& sceneGlobalDescriptorSet = world->getScene().getSceneGlobalDescriptorSet();
		meshDrawer = std::make_unique<MeshDrawer>(*device, *drawContext);
		skyDrawer = std::make_unique<SkyDrawer>(*device, *drawContext);
		fxDrawer = std::make_unique<FxDrawer>(*device, *drawContext);
		debugDrawer = std::make_unique<DebugDrawer>(*device, *drawContext);
		//planetDrawer = std::make_unique<PlanetDrawer>(*device, *world, baseFormats, renderSettings.sampleCountMSAA);
		editor->initEditorUI(*device, *drawContext);
		viewportDrawer = std::make_unique<ViewportDrawer>(*device, *drawContext);
		drawContext->viewportDrawer = viewportDrawer.get();
	}

	void EngineApplication::setupDefaultInputs()
	{
		InputSystem& inputSys = window->input;

		inputSys.captureMouseCursor(false);

		moveForwardInput = inputSys.addInputAxis().addKeyBinding({ KeyBinding(GLFW_KEY_W, 1), KeyBinding(GLFW_KEY_S, -1) });
		moveSidewaysInput = inputSys.addInputAxis().addKeyBinding({ KeyBinding(GLFW_KEY_D, 1), KeyBinding(GLFW_KEY_A, -1) });
		moveUpDownInput = inputSys.addInputAxis().addKeyBinding({ KeyBinding(GLFW_KEY_R, 1), KeyBinding(GLFW_KEY_F, -1) });
		moveFasterInput = inputSys.addInputAxis().addKeyBinding({ KeyBinding(GLFW_KEY_LEFT_SHIFT, 1), KeyBinding(GLFW_KEY_LEFT_CONTROL, -1) });
		exitApplicationEvent = inputSys.addInputEvent().addKeyBinding(KeyBinding(GLFW_KEY_ESCAPE));
		toggleFullscreenEvent = inputSys.addInputEvent(EInputEvent::COMBO).addKeyBinding({ KeyBinding(GLFW_KEY_LEFT_ALT), KeyBinding(GLFW_KEY_ENTER) });
	}

	void EngineApplication::onSwapchainCreated()
	{
		// fxDrawer uses swapchain image count, since it samples from the swapchain attachments, so it must be recreated together with the swapchain
		setupDrawers();
	}

	void EngineApplication::render(const FrameContext& f)
	{
		f.scene->updateDescriptors(f.bufferIndex, f.delta);
		f.scene->updateInstanceData(f.bufferIndex);

		debugDrawer->removeDebugBoxes();
		//debugDrawer->addDebugBox(Vec(static_cast<float>(f.scene->getSectorSize())), Vec(0.f), Vec(0.f, 0.f, .8f), 0.5f);
		
		// RENDER BASE PASS
		renderer->beginRenderingBase(f.commandBuffer); 

		// render sky sphere
		skyDrawer->render(f);

		// render planet (experimental)
		//planetDrawer->render(f.commandBuffer, f.bufferIndex, f.camera->transform, f.delta);

		// render meshes
		meshDrawer->render(f);

		debugDrawer->render(f);

		editor->renderUI(*device.get(), f, *drawContext);

		renderer->endRendering(f.commandBuffer);

		// RENDER FX PASS
		fxDrawer->render(f);

		// RENDER VIEWPORT (POST-FX PASS)
		viewportDrawer->render(f);

		// submit command buffer
		renderer->endFrame(); 
		f.camera->setAspectRatio(renderer->getSwapchainAspectRatio());
	}

	void EngineApplication::inputTick(double delta)
	{
		window->input.updateInputs(); // get new input state
		window->pollEvents(delta); // process events in window queue
		window->refreshFullscreenState();

		exitApplication = exitApplicationEvent.consume();
		if (toggleFullscreenEvent.consume()) 
		{
			device->waitIdle();
			window->toggleFullscreen();
		}
	}

	void EngineApplication::moveCamera(Camera& camera)
	{
		float forward = moveForwardInput;
		float side = moveSidewaysInput;
		float up = moveUpDownInput;
		float faster = moveFasterInput;
		auto mouse = window->input.getMouseDelta();
		camera.moveInPlaneXY(mouse, forward, side, up, faster, static_cast<float>(engineClock.getDelta()));
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