#include "DemoGame.h"
#include "shared/IGame.h"

#include "shared/Transform.h"
#include "game/MeshNode.h"

#include <iostream>
#include <vector>



void spawn(DemoGame& g, xyz p)
{
	g.demoMeshes.push_back(g.spawnNode<EngineInterface::MeshNode>());
	g.demoMeshes.back()->setMesh();
	Transform t = g.demoMeshes.back()->getTransform();
	t.scale = 60;
	t.translation.x = p.x;
	t.translation.y = p.y;
	t.translation.z = p.z;
	g.demoMeshes.back()->setTransform(t);
}

void DemoGame::onLoad()
{
	std::cout << "Game DLL onLoad called\n";

	for (double x = 0; x < 12; x++)
		for (double y = 0; y < 12; y++)
			for (double z = 0; z < 12; z++)
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
		for (double j = 0; j < 12; j++)
		{
			if (i < spawnPositions.size())
				spawn(*this, spawnPositions[i]);
			i++;
		}
		tickTimer = 0.4;
	}


	//Transform t = demoMeshes.back()->getTransform();
	//if (t.scale.x < 100)
	//t.scale += dt * 15;
	//demoMeshes.back()->setTransform(t);
}

void DemoGame::onUnload()
{
	std::cout << "Game DLL onUnload called\n";
}

// define the factory function that instantiates the DemoGame class
GAME_MAIN_FACTORY(DemoGame)