#include "DemoGame.h"
#include "shared/IGame.h"

void DemoGame::onLoad()
{
	std::cout << "Game DLL onLoad called\n";
}

void DemoGame::tick(double dt)
{
	double x = 0;
	double y = 0;
	engine->getMousePosition(x, y);
	std::cout << "(game dll) mouse position x: " << x << " y: " << y << "\n";
}

void DemoGame::onUnload()
{
	std::cout << "Game DLL onUnload called\n";
}

// define the factory function that instantiates the DemoGame class
GAME_MAIN_FACTORY(DemoGame)