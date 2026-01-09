#include "Engine.h"

#include "Core/GPU/Material.h"
#include "Core/GPU/Buffer.h"
#include "Core/GPU/Image.h"

#include <stdexcept>
#include <array>
#include <iostream>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace EngineCore
{
	void EngineApplication::startExecution()
	{
		renderer.swapchainCreatedCallback = std::bind(&EngineApplication::onSwapchainCreated, this);

		// temporary single-camera setup
		camera = Camera(85.f, 10.f, 10000 * 100.f);
		camera.transform.rotation = glm::vec3(0.f, 0.f, 0.f);
		camera.transform.translation = glm::vec3(0.f, 0.f, 150.f);

		setupDescriptors();
		loadDemoScene();
		setupDrawers();
		setupDefaultInputs();

		// window event loop
		while (!window.getCloseWindow())
		{
			window.input.resetInputValues(); // reset input values
			window.input.updateBoundInputs(); // get new input states
			window.pollEvents(); // process events in window queue
			render(); // render frame
		}

		// window pending close, wait for GPU
		vkDeviceWaitIdle(device.device());
	}

	void EngineApplication::setupDescriptors() 
	{
		// demo textures
		marsTexture = std::make_unique<Image>(device, makePath("Textures/mars6k_v2.jpg"));
		spaceTexture = std::make_unique<Image>(device, makePath("Textures/space.png"));

		UBO_Struct ubo1{};
		ubo1.add(uelem::mat4); // MVP matrix
		dset.addUBO(ubo1, device);
		// as the demo textures will never be overwritten from the CPU, only one buffer is needed for each, so the view can simply be duplicated
		ImageArrayDescriptor demoTextureArray{};
		demoTextureArray.addImage(std::vector<VkImageView>(EngineSwapChain::MAX_FRAMES_IN_FLIGHT, marsTexture->getView()));
		demoTextureArray.addImage(std::vector<VkImageView>(EngineSwapChain::MAX_FRAMES_IN_FLIGHT, spaceTexture->getView()));
		dset.addImageArray(demoTextureArray);
		dset.addSampler(marsTexture->sampler);
		dset.finalize();
	}

	void EngineApplication::loadDemoScene()
	{
		world.createDemoSectorContent();
	}

	void EngineApplication::setupDrawers() 
	{
		const auto& baseFormats = renderer.getBasePassFormats();
		const auto& fxFormats = renderer.getFxPassFormats();

		meshDrawer = std::make_unique<MeshDrawer>(device);
		skyDrawer = std::make_unique<SkyDrawer>(device, dset, baseFormats, renderSettings.sampleCountMSAA);
		fxDrawer = std::make_unique<FxDrawer>(device, dset, fxFormats, renderer.getFxPassInputImageViews(), renderer.getFxPassInputDepthImageViews());
		uiDrawer = std::make_unique<InterfaceDrawer>(device, baseFormats, renderSettings.sampleCountMSAA);
		debugDrawer = std::make_unique<DebugDrawer>(device, dset, baseFormats, renderSettings.sampleCountMSAA);
	}

	void EngineApplication::setupDefaultInputs()
	{
		InputSystem& inputSys = window.input;

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

	void EngineApplication::render() 
	{
		if (auto commandBuffer = renderer.beginFrame())
		{
			const uint32_t frameIndex = renderer.getFrameIndex(); // current framebuffer index
			engineClock.measureFrameDelta(frameIndex);

			moveCamera();
			camera.testValue += window.input.getAxisValue(4) * engineClock.getDelta() * 2.f;

			world.sectorUpdate(camera);
			debugDrawer->removeDebugBoxes();
			debugDrawer->addDebugBox(Vec(world.getSectorSize()), world.getLocalSectorOriginAbsolute(), Vec(0.f, 0.f, .8f), 0.5f);
			
			updateDescriptors(frameIndex);

			renderer.beginRenderingBase(commandBuffer); // BASE PASS (dynamic rendering)

			// render sky sphere
			skyDrawer->renderSky(commandBuffer, dset.getDescriptorSet(frameIndex), camera.transform.translation);
			// render meshes
			meshDrawer->renderMeshes(commandBuffer, world, engineClock.getDelta(), engineClock.getElapsed(), frameIndex,
										dset.getDescriptorSet(frameIndex), getProjectionViewMatrix());

			debugDrawer->render(commandBuffer, renderer);

			renderer.endRendering(commandBuffer);

			fxDrawer->render(commandBuffer, renderer);

			renderer.endFrame(); // submit command buffer
			camera.setAspectRatio(renderer.getSwapchainAspectRatio());
		}
	}

	void EngineApplication::updateDescriptors(uint32_t frameIndex)
	{
		glm::mat4 pvm{ 1.f };
		pvm = getProjectionViewMatrix();
		dset.writeUBOMember(0, pvm, UBO_Layout::ElementAccessor{ 0, 0, 0 }, frameIndex);

		// update material-specific descriptors on mesh
		glm::vec3 camPos = camera.transform.translation;

		lightPos.y -= 50.f * engineClock.getDelta();
		float roughness = 0.15f;
		if (world.getLoadedSectors().size() && world.getPersistentSector().primitives.size() > 0)
		{
			auto& meshDset = *world.getPersistentSector().primitives[0]->getMaterial()->getMaterialSpecificDescriptorSet();
			meshDset.writeUBOMember(0, camPos, UBO_Layout::ElementAccessor{ 0, 0, 0 }, frameIndex);
			meshDset.writeUBOMember(0, lightPos, UBO_Layout::ElementAccessor{ 1, 0, 0 }, frameIndex);
			meshDset.writeUBOMember(0, roughness, UBO_Layout::ElementAccessor{ 2, 0, 0 }, frameIndex);
		}
	}

	void EngineApplication::moveCamera()
	{
		auto mf = window.input.getAxisValue(0);
		auto mr = window.input.getAxisValue(1);
		auto mu = window.input.getAxisValue(2);
		auto xs = window.input.getAxisValue(3) > 0 ? true : false;
		auto lookInput = window.input.getMouseDelta();
		camera.moveInPlaneXY(lookInput, mf, mr, mu, xs, engineClock.getDelta());
	}

	glm::mat4 EngineApplication::getProjectionViewMatrix(bool inverse)
	{
		glm::mat4 projectionMatrix = camera.getProjectionMatrix();
		if (!inverse)
		{
			return projectionMatrix * camera.blenderToVulkanMatrix1 * camera.blenderToVulkanMatrix2 * camera.getViewMatrix();
		}
		else
		{
			return glm::inverse(camera.getViewMatrix()) * glm::inverse(camera.blenderToVulkanMatrix1) * glm::inverse(camera.blenderToVulkanMatrix2) * glm::inverse(projectionMatrix);
		}
	}

	glm::vec3 EngineApplication::unproject(glm::vec3 point)
	{
		glm::vec4 v = getProjectionViewMatrix(true) * glm::vec4(point.x, point.y, point.z, 1);
		return glm::vec3(v.x, v.y, v.z) / v.w;
	}

} 