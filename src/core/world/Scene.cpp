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
#include "core/types/Math.h"


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

		// create a basic camera
		currentCamera = std::make_shared<EngineCore::Camera>(CameraSettings{ .fieldOfViewDeg = 85, .nearDistance = 10, .farDistance = 16000 * 100 });
		currentCamera->transform.rotation = { 0.f, 0.f, 0.f };
		currentCamera->transform.translation = { 0.f, 0.f, 0.f };

		// scene global descriptors
		UBO_Struct ubo1{};
		ubo1.add(uelem::mat4); // MVP matrix
		sceneGlobalDescriptorSet->addUBO(ubo1, device);
		sceneGlobalDescriptorSet->finalize();

		// initialize texture manager and instance buffer (SSBO)
		textureManager = std::make_unique<EngineCore::BindlessTextureManager>(device);
		instanceBuffer = std::make_unique<EngineCore::InstanceBuffer>(device);

		// load background texture
		bgTexture = std::make_unique<Image>(device, makePath("textures/space2.png"));
		textureManager->registerTexture(bgTexture);
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
	}

	void Scene::updateInstanceData(uint32_t frameIndex)
	{
		for (Sector* sector : getLoadedSectors())
		{
			for (EngineNodeData* eNode : sector->nodes().getMeshes())
			{
				const Transform& t = eNode->engineTransform;
				// transforms the position from its local sector frame into the sector frame where the camera is
				const Vec meshPosRelative = Math::calculateRelativePositionForRendering(t, getLocalSectorCoordinate());
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

	void Scene::updateNodes()
	{
		// TODO: this should be done for all nodes, not just meshes
		for (Sector* sector : getLoadedSectors())
		{
			for (EngineNodeData* nodeData : sector->nodes().getMeshes())
			{
				// update the engine-side node transform with data from the game
				nodeData->updateTransformFromGame();
			}
		}
	}

	void Scene::physicsTick()
	{
		for (Sector* sector : getLoadedSectors())
		{
			sector->physicsTick();
		}
	}

	void Scene::gamePostPhysicsUpdate()
	{
		for (Sector* sector : getLoadedSectors())
		{
			sector->gamePostPhysicsUpdate();
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

	void Scene::loadSector(const SectorCoord& coord)
	{
		// TODO: load sectors from disk
	}

	const SectorCoord& Scene::getLocalSectorCoordinate() const
	{
		return *localSectorCoord.get();
	}

	void Scene::setLocalSectorCoordinate(const SectorCoord& coordNew)
	{
		*localSectorCoord.get() = coordNew;
		currentCamera->transform.sector = coordNew;
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
		/*TODO: 
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

