#include "core/world/Scene.h"
#include "core/gpu/Device.h"
#include "core/world/Sector.h"
#include "core/world/SectorContainer.h"
#include "core/engine/Camera.h"
#include "core/gpu/Material.h"
#include "core/gpu/Buffer.h"
#include "core/gpu/Image.h"
#include "core/gpu/Descriptors.h"
#include "core/gpu/descriptors/DescriptorSetLayout.h"
#include "core/gpu/descriptors/DescriptorPool.h"
#include "core/gpu/descriptors/BindlessTextureManager.h"
#include "core/gpu/descriptors/InstanceBuffer.h"
#include "core/nodes/EngineNodeData.h"
#include "core/world/NodeContainer.h"
#include "core/engine/Engine.h"
#include "core/render/Renderer.h"
#include "core/gpu/Swapchain.h"
#include "core/types/glm_conversions.h"
#include "core/include/shared/Transform.h"
#include "core/nodes/MeshCache.h"


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
		initGlobalDescriptorSet();
		sectors = std::make_unique<WorldSystem::SectorContainer>();
		localSectorCoord = std::make_unique<SectorCoord>();
		meshCache = std::make_unique<EngineCore::MeshCache>();
	}
	
	EngineCore::DescriptorSet& Scene::getSceneGlobalDescriptorSet() const
	{
		return *sceneGlobalDescriptorSet.get(); 
	}

	EngineCore::Camera& Scene::getCurrentCamera() const
	{
		return *currentCamera.get();
	}

	Sector* Scene::getSector(const SectorCoord& coord, const ESectorLookup& mode) const
	{
		return sectors->getOrCreateSector(coord, mode);
	}

	void Scene::initGlobalDescriptorSet()
	{
		using namespace EngineCore;
		using namespace Nodes;

		// create a basic camera
		currentCamera = std::make_shared<EngineCore::Camera>(CameraSettings{ .fieldOfViewDeg = 85, .nearDistance = 10, .farDistance = 10000 * 100 });
		currentCamera->transform.rotation = { 0.f, 0.f, 0.f };
		currentCamera->transform.translation = { 0.f, 0.f, 150.f };

		// demo textures (remove these later)
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

		// initialize texture manager and instance buffer (SSBO)
		textureManager = std::make_unique<EngineCore::BindlessTextureManager>(device);
		instanceBuffer = std::make_unique<EngineCore::InstanceBuffer>(device);
	}

	std::vector<VkDescriptorSetLayout> Scene::getDescriptorSetLayouts() const
	{
		return {
			sceneGlobalDescriptorSet->getLayout(),
			textureManager->getDescriptorSetLayout()
		};
	}

	std::vector<VkDescriptorSet> Scene::getDescriptorSets(uint32_t frameIndex) const
	{
		return {
			sceneGlobalDescriptorSet->getDescriptorSet(frameIndex),
			textureManager->getDescriptorSet()
		};
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
		/* TODO: ASAP!
		if (getLoadedSectors().size() && getPersistentSector().nodes.size() > 0)
		{
			glm::vec3 camPosRelative{}; // TODO: this can be removed, now using camera-relative rendering in the shader
			auto& meshDset = *getPersistentSector().nodes[0]->getMaterial()->getMaterialSpecificDescriptorSet();
			meshDset.writeUBOMember(0, camPosRelative, EngineCore::UBO_Layout::ElementAccessor{ 0, 0, 0 }, frameIndex);
			meshDset.writeUBOMember(0, lightPos, EngineCore::UBO_Layout::ElementAccessor{ 1, 0, 0 }, frameIndex);
			meshDset.writeUBOMember(0, roughness, EngineCore::UBO_Layout::ElementAccessor{ 2, 0, 0 }, frameIndex);
		}*/
	}

	void Scene::updateInstanceData(uint32_t frameIndex)
	{
		for (Sector* sector : getLoadedSectors())
		{
			for (EngineNodeData* eNode : sector->nodes().getMeshes())
			{
				const Transform& t = eNode->engineTransform;
				const Vec meshPosRelative = WorldSystem::calculateRelative(t.translation, t.sector, getLocalSectorCoordinate());
				const auto modelMatrix = EngineCore::cglm::makeMatrixQ(t.rotation, t.rotation_w, t.scale, meshPosRelative);
				const auto normalMatrix = glm::transpose(glm::inverse(modelMatrix));
				
				instanceBuffer->addInstanceData(
					EngineCore::InstanceData
					{
						.modelMatrix = modelMatrix,
						.normalMatrix = normalMatrix,
						.albedoTexIdx = 0,
						.normalTexIdx = 0,
						.roughnessTexIdx = 0
					});
			}
		}
		instanceBuffer->pushBufferToGPU(frameIndex);
	}

	void Scene::physicsTick()
	{
		// TODO: ASAP
		//for (auto& sector : sectors)
		//{
		//	sector->physicsTick();
		//}
	}

	void Scene::sectorUpdate(EngineCore::Camera& camera)
	{
		if (updateSectorCoord(camera.transform.translation))
		{
			// new local sector entered
			// TODO: ASAP - this should not be commented out
			//loadSector(getLocalSectorCoordinate());
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

	/* TODO: ASAP!
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
	}*/

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
		/*TODO: ASAP
		auto it = std::remove_if(sectors.begin(), sectors.end(), [coord](const std::unique_ptr<Sector>& s) { return s->coordinates == coord; });
		assert(it != sectors.end() && "attempted to remove an unknown world sector");
		assert(it->get()->coordinates != getLocalSectorCoordinate() && "attempted to remove the local world sector");
		sectors.erase(it, sectors.end());
		*/
	}

	std::vector<Sector*> Scene::getLoadedSectors() const
	{
		// TODO: this doesn't account for "culled" sectors
		return sectors->getLoadedSectors();
	}

}

