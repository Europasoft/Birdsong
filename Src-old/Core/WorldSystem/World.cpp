#include "Core/GPU/Device.h"
#include "Core/WorldSystem/World.h"
#include "Core/WorldSystem/Sector.h"
#include "Core/Camera.h"
#include "Core/Primitive.h"
#include "Core/GPU/Material.h"
#include "Core/GPU/Buffer.h"
#include "Core/GPU/Image.h"
#include "Core/Engine.h"

#include <cmath>
#include <algorithm>
#include <iostream>


namespace WorldSystem
{

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
		// create a basic camera
		currentCamera = std::make_shared<EngineCore::Camera>(85.f, 10.f, 10000 * 100.f);
		currentCamera->transform.rotation = glm::vec3(0.f, 0.f, 0.f);
		currentCamera->transform.translation = glm::vec3(0.f, 0.f, 150.f);

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
		for (size_t i = 0; i < 1; i++)
		{
			EngineCore::Primitive::MeshBuilder builder{};
			builder.loadFromFile(makePath("Meshes/teapot.obj")); // TODO: hardcoded path
			sector.primitives.push_back(std::make_unique<EngineCore::Primitive>(device, builder));
			sector.primitives.back()->getTransform().translation = Vec{ 17.f + (i * 17.f), 0.f, 0.f };
			sector.primitives.back()->getTransform().scale = 30.f;
			if (i == 0)
			{
				sector.primitives.back()->getTransform().scale *= 5.f; // scale up the second mesh
				sector.primitives.back()->getTransform().translation.x += 1500.f;
				sector.primitives.back()->getTransform().rotation.z += 95.f;
			}
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
		for (size_t i = 0; i < sector.primitives.size(); i++)
		{
			// TODO: materials should automatically include the layout of their own set (if present) on construct!!!
			EngineCore::MaterialCreateInfo matInfo(shader, std::vector<VkDescriptorSetLayout>{ sceneGlobalDescriptorSet->getLayout(), matSet->getLayout() },
				engine.getRenderSettings().sampleCountMSAA, engine.getRenderer().getBasePassFormats(), sizeof(EngineCore::ShaderPushConstants::MeshPushConstants));
			matInfo.shadingProperties.cullModeFlags = VK_CULL_MODE_NONE;

			sector.primitives[i]->setMaterial(matInfo);
			sector.primitives[i]->getMaterial()->setMaterialSpecificDescriptorSet(matSet); // TODO: better way to create material-specific sets
		}
	}

	void Scene::update(uint32_t frameIndex, double deltaTime)
	{
		auto& cam = getCurrentCamera();

		glm::mat4 pvm{ 1.f };
		pvm = cam.getProjectionViewMatrix();
		sceneGlobalDescriptorSet->writeUBOMember(0, pvm, EngineCore::UBO_Layout::ElementAccessor{ 0, 0, 0 }, frameIndex);

		// update material-specific descriptors on mesh
		const auto& cameraSector = getLocalSectorCoordinate();
		const float S = static_cast<float>(SECTOR_SIZE);

		lightPos.y -= 50.f * static_cast<float>(deltaTime);
		float roughness = 0.15f;
		if (getLoadedSectors().size() && getPersistentSector().primitives.size() > 0)
		{
			glm::vec3 camPosRelative{}; // TODO: this can be removed, now using camera-relative rendering in the shader
			auto& meshDset = *getPersistentSector().primitives[0]->getMaterial()->getMaterialSpecificDescriptorSet();
			meshDset.writeUBOMember(0, camPosRelative, EngineCore::UBO_Layout::ElementAccessor{ 0, 0, 0 }, frameIndex);
			meshDset.writeUBOMember(0, lightPos, EngineCore::UBO_Layout::ElementAccessor{ 1, 0, 0 }, frameIndex);
			meshDset.writeUBOMember(0, roughness, EngineCore::UBO_Layout::ElementAccessor{ 2, 0, 0 }, frameIndex);
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

		const float S = static_cast<float>(SECTOR_SIZE);
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
			(sector.x * SECTOR_SIZE) + offset.x, 
			(sector.y * SECTOR_SIZE) + offset.y, 
			(sector.z * SECTOR_SIZE) + offset.z);
	}

	Vec Scene::getLocalSectorOriginAbsolute() const
	{
		return sectorToAbsolute(getLocalSectorCoordinate(), Vec::zero());
	}

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

	
	World::World(EngineCore::EngineDevice& device, EngineCore::EngineApplication& engine)
		: device{ device }, engine{ engine }
	{
	}

	Scene& World::getScene() const
	{
		return *currentScene.get();
	}
	
	void World::loadDemoScene()
	{
		currentScene = std::make_unique<Scene>(device, engine);
		currentScene->setupDemoScene();
	}
    

}

