#include "core/world/World.h"
#include "core/world/Scene.h"
#include "core/gpu/Device.h"

#include <cmath>
#include <algorithm>
#include <iostream>


namespace WorldSystem
{
	World::~World()
	{
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

