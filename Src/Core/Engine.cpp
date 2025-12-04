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
		//camera.transform.translation.x = -8.f;

		setupDescriptors();
		loadDemoScene();
		setupDrawers();
		setupDefaultInputs();
		//applyDemoMaterials();

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
	
	void EngineApplication::loadDemoScene()
	{
		world.createDemoSectorContent();

		// MOVED TO WORLD SECTOR SYSTEM
		// mars
		//Primitive::MeshBuilder builder{};
		//builder.loadFromFile(makePath("Meshes/mars.obj")); // TODO: hardcoded path
		//loadedMeshes.push_back(std::make_unique<Primitive>(device, builder));
		//loadedMeshes.back()->getTransform().translation = Vec{300.f, 0.f, 0.f};
		//loadedMeshes.back()->getTransform().scale = 120.f;

		/*builder.loadFromFile("G:/VulkanDev/VulkanEngine/Core/DevResources/Meshes/sphere.obj");
		loadedMeshes.push_back(new Primitive(device, builder)); 
		loadedMeshes[1]->useFakeScale = true;//FakeScaleTest082 testing on mesh at index 1
		loadedMeshes[1]->getTransform().translation.x = 0.f;
		loadedMeshes[1]->getTransform().scale = 1.f;*/
	}

	void EngineApplication::setupDescriptors() 
	{
		// demo textures
		marsTexture = std::make_unique<Image>(device, makePath("Textures/mars6k_v2.jpg"));
		spaceTexture = std::make_unique<Image>(device, makePath("Textures/space.png"));

		UBO_Struct ubo1{};
		ubo1.add(uelem::mat4); // MVP matrix
		//ubo1.add(std::vector{ uelem::scalar, uelem::vec3 }, 2); // test
		dset.addUBO(ubo1, device);
		// as the demo textures will never be overwritten from the CPU, only one buffer is needed for each, so the view can simply be duplicated
		ImageArrayDescriptor demoTextureArray{};
		demoTextureArray.addImage(std::vector<VkImageView>(EngineSwapChain::MAX_FRAMES_IN_FLIGHT, marsTexture->getView()));
		demoTextureArray.addImage(std::vector<VkImageView>(EngineSwapChain::MAX_FRAMES_IN_FLIGHT, spaceTexture->getView()));
		dset.addImageArray(demoTextureArray);
		dset.addSampler(marsTexture->sampler);
		dset.finalize();
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
		//debugDrawer->addDebugBox(Vec(100.f), Vec::zero(), Vec(0.f, 0.f, .8f), 0.5f);
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

	void EngineApplication::applyDemoMaterials()
	{
		/*
		* MOVE TO WORLD SECTOR SYSTEM*/

		// create demo material
		//ShaderFilePaths shader(makePath("Shaders/shader.vert.spv"), makePath("Shaders/shader.frag.spv"));
		//loadedMeshes[0]->setMaterial(MaterialCreateInfo(shader, std::vector<VkDescriptorSetLayout>{ dset.getLayout() }, 
		//								renderSettings.sampleCountMSAA, renderer.getBaseRenderpass().getRenderpass(), sizeof(ShaderPushConstants::MeshPushConstants)));


		// descriptor set must be initialized before using its layout
		//UBO_Struct ubo_g{};
		//ubo_g.add(uelem::vec3); // camera position
		//ubo_g.add(uelem::vec3); // light position
		//ubo_g.add(uelem::scalar); // roughness
		//auto matSet = std::make_shared<DescriptorSet>(device);
		//matSet->addUBO(ubo_g, device);
		//matSet->finalize(); // create material-specific descriptor set

		//ShaderFilePaths shader2(makePath("Shaders/shader.vert.spv"), makePath("Shaders/pbr.frag.spv"));
		//for (size_t i = 1; i < loadedMeshes.size(); i++)
		//{
		//	// TODO: materials should automatically include the layout of their own set (if present) on construct!!!
		//	MaterialCreateInfo matInfo(shader2, std::vector<VkDescriptorSetLayout>{ dset.getLayout(), matSet->getLayout() },
		//				renderSettings.sampleCountMSAA, renderer.getBaseRenderpass().getRenderpass(), sizeof(ShaderPushConstants::MeshPushConstants));
		//	matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;
		//	loadedMeshes[i]->setMaterial(matInfo);
		//	loadedMeshes[i]->getMaterial()->setMaterialSpecificDescriptorSet(matSet); // TODO: better way to create material-specific sets
		//}
	}

	void EngineApplication::onSwapchainCreated()
	{
		// fxDrawer uses swapchain image count, since it samples from the swapchain attachments, so it must be recreated together with the swapchain
		setupDrawers();
	}

	/*
	void EngineApplication::simulateDistanceByScale(const StaticMesh& mesh, const Transform& cameraTransform)
	{
		// old, regular-precision version
		//const auto actualScale = mesh.transform.scale.x; // uniform scale required! (for now)
		//const auto actualLoc = mesh.transform.translation;
		//const auto camLoc = cameraTransform.translation;
		//
		//const float max = 2.f;
		//const auto dst = Vec::distance(actualLoc, camLoc); std::cout << "\n" << dst;
		//
		//
		//auto dir = Vec::direction(camLoc, actualLoc);
		//const auto offset = dst - max; // offset between actual location and render location, along direction
		//const auto scale = max / dst; // fake scale, decided by observer distance
		//
		//simDistOffsets.translation = actualLoc + (dir * -offset);
		//simDistOffsets.scale = actualScale * scale;

		const float actualScale_f = 696340 * 2000;
		const float actualLoc_f = 2000000 * 1000;

		double actualScale = (double)actualScale_f;
		auto actualLoc = Vector3D<double>{ (double)actualLoc_f, 0.0, 0.0 };
		const auto camLoc = Vector3D<double>{ cameraTransform.translation.x, cameraTransform.translation.y, cameraTransform.translation.z };

		const double max = 8.0;
		const double dst = ddist(actualLoc, camLoc);
		const auto dir = ddir(camLoc, actualLoc);
		const auto offset = dst - max;
		const auto scale = max / dst;

		const Vector3D<double> t = actualLoc + (dir * -offset);
		const double s = actualScale * scale;

		simDistOffsets.translation = { (float)t.x, (float)t.y, (float)t.z };
		simDistOffsets.scale = (float)s;
	}

	double EngineApplication::ddist(const Vector3D<double>& a, const Vector3D<double>& b)
	{
		const double dsq = pow(b.x - a.x, 2) + pow(b.y - a.y, 2) + pow(b.z - a.z, 2);
		return sqrt(dsq);
	}

	Vector3D<double> EngineApplication::ddir(const Vector3D<double>& a, const Vector3D<double>& b) 
	{
		Vector3D<double> k = b - a; // direction
		// normalization to unit
		const auto sum = (k.x * k.x) + (k.y * k.y) + (k.z * k.z);
		const auto f = 1.0 / sqrt(sum);
		return { k.x * f, k.y * f, k.z * f };
	}
		*/
	/*void EngineApplication::applyWorldOriginOffset(Transform& cameraTransform)
	{
		const auto threshold = 250000.f;
		auto& tft = cameraTransform.translation;

		const auto x = std::max(std::max(abs(tft.x), abs(tft.y)), abs(tft.z));
		if (x < threshold) { return; }

		const auto nw = cameraTransform.translation;

		std::cout << "\n\n\n\n\n rebasing world origin to x:" << nw.x << " y:" << nw.y << " z:" << nw.z;

		cameraTransform.translation = {0.f,0.f,0.f};
		if (loadedMeshes.size() > 0) 
		{
			for (auto& m : loadedMeshes) { m->getTransform().translation = m->getTransform().translation - nw; }
		}
	}*/


	void EngineApplication::render() 
	{
		if (auto commandBuffer = renderer.beginFrame())
		{
			const uint32_t frameIndex = renderer.getFrameIndex(); // current framebuffer index
			engineClock.measureFrameDelta(frameIndex);

			moveCamera();
			camera.testValue += window.input.getAxisValue(4) * engineClock.getDelta() * 2.f;
			//skyDrawer->skyMeshScale = camera.testValue;
			//loadedMeshes.back()->getTransform().translation.x += window.input.getAxisValue(4) * engineClock.getDelta() * 1.f;
			//std::cout << "\nx: " << loadedMeshes.back()->getTransform().translation.x;
			//std::cout << "\ntest value: " << camera.testValue;

			testMoveObjectWithMouse();

			world.sectorUpdate(camera);
			//std::cout << camera.transform.translation.x * 0.00001 << "\n";
			debugDrawer->removeDebugBoxes();
			debugDrawer->addDebugBox(Vec(world.getSectorSize()), world.getLocalSectorOriginAbsolute(), Vec(0.f, 0.f, .8f), 0.5f);
			
			updateDescriptors(frameIndex);

			renderer.beginRenderingBase(commandBuffer); // BASE PASS (dynamic rendering)

			// render sky sphere
			skyDrawer->renderSky(commandBuffer, dset.getDescriptorSet(frameIndex), camera.transform.translation);
			//simulateDistanceByScale(*loadedMeshes[1].get(), camera.transform); //FakeScaleTest082
			// render meshes
			meshDrawer->renderMeshes(commandBuffer, world, engineClock.getDelta(), engineClock.getElapsed(), frameIndex,
										dset.getDescriptorSet(frameIndex), getProjectionViewMatrix(), simDistOffsets); //FakeScaleTest082

			debugDrawer->render(commandBuffer, renderer);

			//uiDrawer->render(commandBuffer, window.input.getMousePosition(), renderer.getSwapchainExtent());  // render test UI

			renderer.endRendering(commandBuffer);

			fxDrawer->render(commandBuffer, renderer);

			renderer.endFrame(); // submit command buffer
			camera.setAspectRatio(renderer.getSwapchainAspectRatio());
		}
	}

	void EngineApplication::moveCamera()
	{
		auto mf = window.input.getAxisValue(0);
		auto mr = window.input.getAxisValue(1);
		auto mu = window.input.getAxisValue(2);
		auto xs = window.input.getAxisValue(3) > 0 ? true : false;
		auto lookInput = window.input.getMouseDelta();
		//lookInput = { 0.0, 0.0 }; // this disables mouse camera movement!
		camera.moveInPlaneXY(lookInput, mf, mr, mu, xs, engineClock.getDelta());
		// move with camera, temporary
		//Transform t = camera.transform;
		//t.translation.x += 5.f;
		//loadedMeshes.back()->setTransform(t);
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

	void EngineApplication::testMoveObjectWithMouse()
	{
		//loadedMeshes.back()->getTransform().scale *= 20.f;//

		// query inputs, this should only be used in 3D editor viewport
		if (window.input.getAxisValue(5) < .1)
		{ 
			movingObjectWithCursor = false;
			return;
		}

		if (world.getLoadedSectors().size() < 2 && world.getLoadedSectors()[1]->primitives.size() < 2)
			return;

		auto& objectToMove = world.getLoadedSectors()[1]->primitives[1];

		if (!movingObjectWithCursor)
		{
			mouseMoveObjectOriginalLocation = objectToMove->getTransform().translation;
			movingObjectWithCursor = true;
		}

		auto mx = ((window.input.getMousePosition().x / window.getExtent().width) * 2) - 1;
		auto my = ((window.input.getMousePosition().y / window.getExtent().height) * 2) - 1;
		//auto p = pvm_inv * glm::vec4(mx, my, planeDistance, 1);
		auto nearPoint = unproject(glm::vec3(mx, my, 0));
		auto farPoint = unproject(glm::vec3(mx, my, 1));
		auto p = nearPoint + (glm::normalize(nearPoint - farPoint) * Vec::distance(mouseMoveObjectOriginalLocation, camera.transform.translation)) * -1.f;

		//std::cout << "\nnear plane distance: " << Vec::distance(camera.transform.translation, nearPoint);
		//std::cout << "\nfar plane distance: " << Vec::distance(camera.transform.translation, farPoint);
		//std::cout << "\n\nfinal x:" << p.x << " y:" << p.y << " z:" << p.z<<
		//	" distance from camera : " << Vec::distance(p, camera.transform.translation);

		bool moveX = window.input.getAxisValue(6) > .1;
		bool moveY = window.input.getAxisValue(7) > .1;
		bool moveZ = window.input.getAxisValue(8) > .1;
		bool moveAll = !moveX && !moveY && !moveZ;

		p.x = moveAll || moveX ? p.x : mouseMoveObjectOriginalLocation.x;
		p.y = moveAll || moveY ? p.y : mouseMoveObjectOriginalLocation.y;
		p.z = moveAll || moveZ ? p.z : mouseMoveObjectOriginalLocation.z;
		
		p.x += 1800.f;// TMP!!!

		objectToMove->setTranslation(p);

		std::cout << "\nfinal x:" << p.x << " y:" << p.y << " z:" << p.z;
	}

	glm::vec3 EngineApplication::unproject(glm::vec3 point)
	{
		glm::vec4 v = getProjectionViewMatrix(true) * glm::vec4(point.x, point.y, point.z, 1);
		return glm::vec3(v.x, v.y, v.z) / v.w;
		//auto pvm_inv = glm::inverse(camera.getViewMatrix()) * basis conversion matrix * glm::inverse(camera.getProjectionMatrix());
	}

	glm::vec3 EngineApplication::getMouseMove3DLocationTest_legacy(float planeDistance, float dist2)
	{
		auto viewportExtent = window.getExtent();
		/*auto x = window.input.getMousePosition().x - viewportExtent.width;
		auto y = window.input.getMousePosition().y - viewportExtent.height;
		glm::vec4 pos = { x / (viewportExtent.width/2), y / (viewportExtent.height/2), 0, 0};
		//
		// using view-projection matrix, ideally should only be calculated once per frame
		pos = glm::inverse((camera.getProjectionMatrix() * basis conversion matrix * camera.getViewMatrix())*planeDistance) * pos;
		return { pos.x, pos.y, pos.z };*/

		//void homogeneous_to_world(vec3 & world, const vec3 & homogeneous, const mat4 & projection, const mat4 & view)
		auto x = ((window.input.getMousePosition().x / viewportExtent.width) * 2) - 1;
		auto y = ((window.input.getMousePosition().y / viewportExtent.height) * 2) - 1;

		//glm::mat4 pvm_inv = glm::inverse(camera.getProjectionMatrix(camera.nearPlane+100, camera.nearPlane+400) * basis conversion matrix * camera.getViewMatrix());
		glm::mat4 pvm_inv; // this line replaces the commented-out one above to indicate that we now use a common wrapper function to get the matrices instead, not real code
		glm::vec4 w = pvm_inv * glm::vec4(x, y, planeDistance, dist2);
		auto worldv = glm::vec3(w) * (1.0f / w.w);
		//std::cout << "\n\nmouse x:" << x << " y:" << y;
		return { worldv.x, worldv.y, worldv.z };
	}

	void EngineApplication::updateDescriptors(uint32_t frameIndex)
	{
		glm::mat4 pvm{ 1.f };
		//pvm = camera.getProjectionMatrix() * basis conversion matrix * camera.getViewMatrix();
		pvm = getProjectionViewMatrix();
		dset.writeUBOMember(0, pvm, UBO_Layout::ElementAccessor{ 0, 0, 0 }, frameIndex);

		//float testScalar1 = 1.f - std::sin(engineClock.getElapsed() * 10.f);
		//float testScalar2 = 1.f - std::sin(engineClock.getElapsed() * 50.f);
		//dset.writeUBOMember(0, testScalar1, UBO_Layout::ElementAccessor{ 1, 0, 0 }, frameIndex);
		//dset.writeUBOMember(0, testScalar2, UBO_Layout::ElementAccessor{ 1, 1, 0 }, frameIndex);

		//applyWorldOriginOffset(camera.transform); //(TODO: ) experimental

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


} 