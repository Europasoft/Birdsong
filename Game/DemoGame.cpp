#include "DemoGame.h"
#include "shared/IGame.h"

#include "shared/Transform.h"
#include "game/MeshNode.h"

#include <iostream>
#include <vector>



void spawn(DemoGame& g, xyz p, EngineInterface::IEngine* engine)
{
	g.demoMeshes.push_back(g.spawnNode<EngineInterface::MeshNode>());
	g.demoMeshes.back()->setMesh();
	Transform t = g.demoMeshes.back()->getTransform();
	t.scale = 60;
	t.translation.x = p.x;
	t.translation.y = p.y;
	t.translation.z = p.z;
	g.demoMeshes.back()->setTransform(t);
	engine->setPhysicsBodyForNode(g.demoMeshes.back().get());
}

static constexpr auto spawnBaseNum = 4;

void DemoGame::onLoad()
{
	std::cout << "Game DLL onLoad called\n";

	for (double x = 0; x < spawnBaseNum; x++)
		for (double y = 0; y < spawnBaseNum; y++)
			for (double z = 0; z < spawnBaseNum; z++)
			{
				spawnPositions.push_back(xyz{ (x + 1) * 1000, y * 1000, z * 1000 });
			}
}

void DemoGame::tick(double dt)
{
	static double tickTimer = 0.1;
	tickTimer -= dt;
	static int i = 0;
	if (tickTimer <= 0 && i < spawnPositions.size())
	{
		for (double j = 0; j < spawnBaseNum; j++)
		{
			if (i < spawnPositions.size())
				spawn(*this, spawnPositions[i], engine);
			i++;
		}
		tickTimer = 0.1;
	}

	/*for (auto& x : demoMeshes)
	{
		Transform t = x->getTransform();
		t.scale += 3 * dt;
		x->setTransform(t);
	}*/
}

void DemoGame::onUnload()
{
	std::cout << "Game DLL onUnload called\n";
}

// define the factory function that instantiates the DemoGame class
GAME_MAIN_FACTORY(DemoGame)