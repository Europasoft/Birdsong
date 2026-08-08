#pragma once
#include "core/types/CommonTypes.h"
#include "core/world/Sector.h"

#include "core/types/vk.h"

#include <stdint.h>
#include <memory>
#include <vector>

namespace EngineCore 
{ 
	class EngineDevice;
	class EngineApplication;
	
	class Image;
	class Camera;
	class DescriptorSet;
	class BindlessTextureManager;
	class InstanceBuffer;
	class MeshCache;
}

namespace WorldSystem
{
	class SectorContainer;
	enum class ESectorLookup : int32_t;

	class Scene
	{
		EngineCore::EngineDevice& device;
		EngineCore::EngineApplication& engine;
	public:
		Scene(EngineCore::EngineDevice& device, EngineCore::EngineApplication& engine);
		~Scene();

		void updateDescriptors(uint32_t frameIndex, double deltaTime);
		void updateInstanceData(uint32_t frameIndex);
		void updateNodes();
		void physicsTick();
		void gamePostPhysicsUpdate();

		EngineCore::DescriptorSet& getSceneGlobalDescriptorSet() const;
		EngineCore::Camera& getCurrentCamera() const;
		EngineCore::InstanceBuffer& getInstanceBuffer() const { return *instanceBuffer.get(); }
		EngineCore::BindlessTextureManager& getTextureManager() const { return *textureManager.get(); }

		// returns layouts for all global descriptor sets
		std::vector<VkDescriptorSetLayout> getDescriptorSetLayouts() const;
		// returns set handles for all global descriptor sets
		std::vector<VkDescriptorSet> getDescriptorSets(uint32_t frameIndex) const;

		Sector* getSector(const SectorCoord& coord, const ESectorLookup& mode) const;

		// currently loaded sectors
		std::unique_ptr<WorldSystem::SectorContainer> sectors;
		std::unique_ptr<SectorCoord> localSectorCoord;

		std::unique_ptr<EngineCore::MeshCache> meshCache;

	protected:
		void initGlobalDescriptorSet();

		std::unique_ptr<EngineCore::DescriptorSet> sceneGlobalDescriptorSet;
		std::unique_ptr<EngineCore::BindlessTextureManager> textureManager;
		std::unique_ptr<EngineCore::InstanceBuffer> instanceBuffer;

		std::shared_ptr<EngineCore::Camera> currentCamera;

		std::unique_ptr<EngineCore::Image> bgTexture; // default background texture

		Vec lightPos{ -20.f, 100.f, 45.f };

		// sector stuff
	public:
		void sectorUpdate(EngineCore::Camera& camera); // checks whether we have moved into a new sector
		void loadSector(const SectorCoord& coord);
		const SectorCoord& getLocalSectorCoordinate() const;
		void setLocalSectorCoordinate(const SectorCoord& coordNew);
		static Vec sectorToAbsolute(const SectorCoord& sector, Vec offset = Vec::zero());
		// returns the real physical location of the current sector center, in world units
		Vec getLocalSectorOriginAbsolute() const;
		uint32_t getSectorSize() const;
		std::vector<Sector*> getLoadedSectors() const;
		
	private:
		bool updateSectorCoord(Vec& pos);
		void forgetSector(const SectorCoord& coord);

	};

}