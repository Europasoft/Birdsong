#include "DemoGame.h"
#include "shared/IGame.h"

void DemoGame::onLoad()
{
	std::cout << "Game DLL onLoad called\n";
}

void DemoGame::tick(float dt)
{
	std::cout << "Game DLL tick called\n";
}

void DemoGame::onUnload()
{
	std::cout << "Game DLL onUnload called\n";
}

// define the factory function that instantiates the DemoGame class
GAME_MAIN_FACTORY(DemoGame)