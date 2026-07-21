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

		// currently loaded sectors, index 0 is the persistent sector
		std::vector<std::unique_ptr<Sector>> sectors;
		std::unique_ptr<SectorCoord> localSectorCoord;

	protected:
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
		Sector& getPersistentSector() const { return *sectors[0].get(); }
	private:
		bool updateSectorCoord(Vec& pos);
		Sector* getSector(const SectorCoord& coord);
		Sector& loadSector(const SectorCoord& sectorPosition);
		void forgetSector(const SectorCoord& coord);

	};

}