#include "core/gpu/Device.h"
#include "core/world/Scene.h"
#include "core/world/Sector.h"
#include "core/engine/Camera.h"
#include "core/nodes/MeshNode.h"
#include "core/gpu/Material.h"
#include "core/gpu/Buffer.h"
#include "core/gpu/Image.h"
#include "core/engine/Engine.h"
#include "core/render/Renderer.h"
#include "core/gpu/Swapchain.h"
#include "core/types/glm_conversions.h"

#include "deps/box3d-cpp/include/b3cpp.h"

#include <cmath>
#include <algorithm>
#include <iostream>


namespace WorldSystem
{
	Scene::~Scene() 
	{
	}

	Scene::Scene(EngineCore::EngineDevice& device, EngineCore::EngineApplication& engine)
		: device{ device }, engine{ engine }
	{
		using namespace EngineCore;
		sceneGlobalDescriptorSet = std::make_unique<DescriptorSet>(device);

		// create the persistent sector
		sectors.push_back(std::make_unique<Sector>(SectorCoord(0, 0, 0)));
		localSectorCoord = std::make_unique<SectorCoord>();
	}

	EngineCore::DescriptorSet& Scene::getSceneGlobalDescriptorSet() const
	{
		return *sceneGlobalDescriptorSet.get();
	}

	EngineCore::Camera& Scene::getCurrentCamera() const
	{
		return *currentCamera.get();
	}

	void Scene::setupDemoScene()
	{
		using namespace EngineCore;
		using namespace Nodes;
		// create a basic camera
		currentCamera = std::make_shared<EngineCore::Camera>(85.f, 10.f, 10000 * 100.f);
		currentCamera->transform.rotation = { 0.f, 0.f, 0.f };
		currentCamera->transform.translation = { 0.f, 0.f, 150.f };

		// demo textures
		marsTexture = std::make_unique<Image>(device, makePath("Textures/mars6k_v2.jpg"));
		spaceTexture = std::make_unique<Image>(device, makePath("Textures/space.png"));

		// scene global descriptors
		UBO_Struct ubo1{};
		ubo1.add(uelem::mat4); // MVP matrix
		sceneGlobalDescriptorSet->addUBO(ubo1, device);
		// as the demo textures will never be overwritten from the CPU, only one buffer is needed for each, so the view can simply be duplicated
		ImageArrayDescriptor demoTextureArray{};
		demoTextureArray.addImage(std::vector<VkImageView>(EngineSwapChain::MAX_FRAMES_IN_FLIGHT, marsTexture->getView()));
		demoTextureArray.addImage(std::vector<VkImageView>(EngineSwapChain::MAX_FRAMES_IN_FLIGHT, spaceTexture->getView()));
		sceneGlobalDescriptorSet->addImageArray(demoTextureArray);
		sceneGlobalDescriptorSet->addSampler(marsTexture->sampler);
		sceneGlobalDescriptorSet->finalize();


		auto& sector = *sectors[0]; // get the persistent sector

		// create 3D primitive(s)
		{
			// TODO: hardcoded path
			Nodes::MeshNode& node = sector.createNode<Nodes::MeshNode>(device);
			node.build("Meshes/axis_cube.obj");
			Transform tf(Vec(5000, 0.f, 0.f), Vec(), Vec(1000.f));
			node.setTransform(tf);
			// TEST: add physics body for mesh
			b3cpp::BodyDef bodyDef;
			bodyDef.type = b3cpp::EBodyType::DynamicBody;
			b3cpp::Body& body = node.addPhysicsBody(bodyDef, sector.getPhysicsWorld());
			b3cpp::BoxHullShape& s = body.createShape<b3cpp::BoxHullShape>();
			s.halfWidthX = 1000;
			s.halfWidthY = 1000;
			s.halfWidthZ = 1000;
			b3cpp::ShapeDef shapeDef;
			shapeDef.density = 5;
			s.activate(shapeDef);
			body.setAngularVelocity({ 0.08, 0.008, 0.01 });
		}

		for (size_t i = 0; i < 8; i++)
		{
			// TODO: hardcoded path
			Nodes::MeshNode& node = sector.createNode<Nodes::MeshNode>(device);
			node.build("Meshes/teapot.obj");
			Transform tf(Vec(1517 + (i * 200.f), (i * 200.f), 0.f), Vec(), Vec(30.f));
			node.setTransform(tf);
			// TEST: add physics body for mesh
			b3cpp::BodyDef bodyDef;
			bodyDef.type = b3cpp::EBodyType::DynamicBody;
			auto& body = node.addPhysicsBody(bodyDef, sector.getPhysicsWorld());
			b3cpp::BoxHullShape& s = body.createShape<b3cpp::BoxHullShape>();
			s.halfWidthX = 200;
			s.halfWidthY = 200;
			s.halfWidthZ = 200;
			b3cpp::ShapeDef shapeDef;
			shapeDef.density = 5;
			s.activate(shapeDef);
			body.setAngularVelocity({ 0.02, 0.006, -0.003 });
			body.setLinearVelocity({ 800, 0, (i * -80.f)});
		}

		// create material-specific descriptor set (the set must be initialized before using its layout)
		EngineCore::UBO_Struct ubo{};
		ubo.add(EngineCore::uelem::vec3); // camera position
		ubo.add(EngineCore::uelem::vec3); // light position
		ubo.add(EngineCore::uelem::scalar); // roughness
		auto matSet = std::make_shared<EngineCore::DescriptorSet>(device);
		matSet->addUBO(ubo, device);
		matSet->finalize(); // create material-specific descriptor set

		// create demo material
		EngineCore::ShaderFilePaths shader(makePath("Shaders/shader.vert.spv"), makePath("Shaders/pbr.frag.spv"));
		for (size_t i = 0; i < sector.nodes.size(); i++)
		{
			// TODO: materials should automatically include the layout of their own set (if present) on construct!!!
			EngineCore::MaterialCreateInfo matInfo(shader, std::vector<VkDescriptorSetLayout>{ sceneGlobalDescriptorSet->getLayout(), matSet->getLayout() },
				engine.getRenderSettings().sampleCountMSAA, engine.getRenderer().getBasePassFormats(), sizeof(EngineCore::ShaderPushConstants::MeshPushConstants));
			matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;

			sector.nodes[i]->setMaterial(matInfo);
			sector.nodes[i]->getMaterial()->setMaterialSpecificDescriptorSet(matSet); // TODO: better way to create material-specific sets
		}
	}

	void Scene::updateDescriptors(uint32_t frameIndex, double deltaTime)
	{
		auto& cam = getCurrentCamera();
		glm::mat4 pvm{ 1.f };
		pvm = cam.getProjectionViewMatrix();
		sceneGlobalDescriptorSet->writeUBOMember(0, pvm, EngineCore::UBO_Layout::ElementAccessor{ 0, 0, 0 }, frameIndex);

		//const auto& cameraSector = getLocalSectorCoordinate();
		const float S = static_cast<float>(Sector::SECTOR_SIZE);
		lightPos.y -= 50.f * static_cast<float>(deltaTime);
		float roughness = 0.15f;
		if (getLoadedSectors().size() && getPersistentSector().nodes.size() > 0)
		{
			glm::vec3 camPosRelative{}; // TODO: this can be removed, now using camera-relative rendering in the shader
			auto& meshDset = *getPersistentSector().nodes[0]->getMaterial()->getMaterialSpecificDescriptorSet();
			meshDset.writeUBOMember(0, camPosRelative, EngineCore::UBO_Layout::ElementAccessor{ 0, 0, 0 }, frameIndex);
			meshDset.writeUBOMember(0, lightPos, EngineCore::UBO_Layout::ElementAccessor{ 1, 0, 0 }, frameIndex);
			meshDset.writeUBOMember(0, roughness, EngineCore::UBO_Layout::ElementAccessor{ 2, 0, 0 }, frameIndex);
		}
	}

	void Scene::physicsTick()
	{
		for (auto& sector : sectors)
		{
			sector->physicsTick();
		}
	}

	void Scene::sectorUpdate(EngineCore::Camera& camera)
	{
		if (updateSectorCoord(camera.transform.translation))
		{
			// new local sector entered
			loadSector(getLocalSectorCoordinate());
		}
	}

	bool Scene::updateSectorCoord(Vec& pos)
	{
		SectorCoord coordNew = getLocalSectorCoordinate();
		bool enteredNewSector = false;

		const float S = static_cast<float>(Sector::SECTOR_SIZE);
		const float halfS = S * 0.5f;

		auto processAxis = [&](float& p, intmax_t& c) 
		{
			intmax_t shift = static_cast<intmax_t>(std::floor((p + halfS) / S));
			if (shift != 0)
			{
				// sector boundary was crossed
				c += shift;
				p -= static_cast<float>(shift) * S; // wraps position back to the relative local frame
				enteredNewSector = true;
			}
		};

		processAxis(pos.x, coordNew.x);
		processAxis(pos.y, coordNew.y);
		processAxis(pos.z, coordNew.z);

		if (enteredNewSector)
		{
			setLocalSectorCoordinate(coordNew);
		}
		return enteredNewSector;
	}

	Sector& Scene::loadSector(const SectorCoord& sectorPosition)
	{
		// TODO: allow loading arbitrary sectors from file
		if (sectorPosition != SectorCoord(0,0,0))
		{
			//std::cout << "sector loading not implemented for " << sectorPosition.x << ", " << sectorPosition.y << ", " << sectorPosition.z;
			return *sectors.back().get();
		}
		else
		{
			
		}

		return *sectors.back().get();
	}

	const SectorCoord& Scene::getLocalSectorCoordinate() const
	{
		return *localSectorCoord.get();
	}

	void Scene::setLocalSectorCoordinate(const SectorCoord& coordNew)
	{
		*localSectorCoord.get() = coordNew;
	}

	Vec Scene::sectorToAbsolute(const SectorCoord& sector, Vec offset)
	{ 
		return Vec(
			(sector.x * Sector::SECTOR_SIZE) + offset.x,
			(sector.y * Sector::SECTOR_SIZE) + offset.y,
			(sector.z * Sector::SECTOR_SIZE) + offset.z);
	}

	Vec Scene::getLocalSectorOriginAbsolute() const
	{
		return sectorToAbsolute(getLocalSectorCoordinate(), Vec::zero());
	}

	uint32_t Scene::getSectorSize() const { return Sector::SECTOR_SIZE; }

	void Scene::forgetSector(const SectorCoord& coord)
	{
		auto it = std::remove_if(sectors.begin(), sectors.end(), [coord](const std::unique_ptr<Sector>& s) { return s->coordinates == coord; });
		assert(it != sectors.end() && "attempted to remove an unknown world sector");
		assert(it->get()->coordinates != getLocalSectorCoordinate() && "attempted to remove the local world sector");
		sectors.erase(it, sectors.end());
	}

	Sector* Scene::getSector(const SectorCoord& coord)
	{
		auto it = std::find_if(sectors.begin(), sectors.end(), [coord](const std::unique_ptr<Sector>& s) { return s->coordinates == coord; });
		if (it == sectors.end()) { return nullptr ; }
		return it->get();
	}

	std::vector<Sector*> Scene::getLoadedSectors() const
	{
		std::vector<Sector*> loadedSectors;
		loadedSectors.reserve(sectors.size());
		for (const auto& sectorPtr : sectors)
		{
			if (sectorPtr && !sectorPtr->isSectorCulled) loadedSectors.push_back(sectorPtr.get());
		}
		return loadedSectors;
	}

}

