#include "DemoGame.h"
#include "shared/IGame.h"

#include "shared/Transform.h"
#include "game/MeshNode.h"

#include <iostream>

void DemoGame::onLoad()
{
	std::cout << "Game DLL onLoad called\n";

	demoMeshes.push_back(spawnNode<EngineInterface::MeshNode>());
	demoMeshes.back()->setMesh();
	std::cout << "Game: Demo mesh spawned\n";
	Transform t = demoMeshes.back()->getTransform();
	t.scale = 60;
	t.translation.x += 1000;
	demoMeshes.back()->setTransform(t);
}

void DemoGame::tick(double dt)
{
	Transform t = demoMeshes.back()->getTransform();
	if (t.scale.x < 100)
	t.scale += dt * 15;
	demoMeshes.back()->setTransform(t);
}

void DemoGame::onUnload()
{
	std::cout << "Game DLL onUnload called\n";
}

// define the factory function that instantiates the DemoGame class
GAME_MAIN_FACTORY(DemoGame)