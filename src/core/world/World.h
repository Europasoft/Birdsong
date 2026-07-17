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
}

namespace WorldSystem
{
	class Scene;

	class World
	{

	public:
		World(EngineCore::EngineDevice& device, EngineCore::EngineApplication& engine);
		~World();

		std::unique_ptr<Scene> currentScene;

		Scene& getScene() const;

		void loadDemoScene();

	private:
		EngineCore::EngineDevice& device;
		EngineCore::EngineApplication& engine;

	};
}