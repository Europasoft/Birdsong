#pragma once
#include "core/types/CommonTypes.h"
#include "core/world/Sector.h"

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

		void setupDemoScene();

		void updateDescriptors(uint32_t frameIndex, double deltaTime);

		void physicsTick();

		EngineCore::DescriptorSet& getSceneGlobalDescriptorSet() const;
		EngineCore::Camera& getCurrentCamera() const;

		Sector* getSector(const SectorCoord& coord, const ESectorLookup& mode) const;

		// currently loaded sectors
		std::unique_ptr<WorldSystem::SectorContainer> sectors;
		std::unique_ptr<SectorCoord> localSectorCoord;

	protected:
		void initGlobalDescriptorSet();

		std::unique_ptr<EngineCore::DescriptorSet> sceneGlobalDescriptorSet;

		std::shared_ptr<EngineCore::Camera> currentCamera;

		/* temporary demo content */
		
		std::unique_ptr<EngineCore::Image> spaceTexture;
		std::unique_ptr<EngineCore::Image> marsTexture;

		Vec lightPos{ -20.f, 100.f, 45.f };

		// sector stuff
	public:
		void sectorUpdate(EngineCore::Camera& camera);// checks whether we have moved into a new sector
		const SectorCoord& getLocalSectorCoordinate() const;
		void setLocalSectorCoordinate(const SectorCoord& coordNew);
		static Vec sectorToAbsolute(const SectorCoord& sector, Vec offset = Vec::zero());
		// returns the real physical location of the current sector center, in world units
		Vec getLocalSectorOriginAbsolute() const;
		uint32_t getSectorSize() const;
		std::vector<Sector*> getLoadedSectors() const;
		//Sector& getPersistentSector() const { return *sectors[0].get(); } TODO: ASAP
	private:
		bool updateSectorCoord(Vec& pos);
		//Sector& loadSector(const SectorCoord& sectorPosition); TODO: ASAP
		void forgetSector(const SectorCoord& coord);

	};

}