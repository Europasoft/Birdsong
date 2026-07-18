#include "GameMain.h"
#include "IGameMacros.h"

void Game::onLoad()
{
	std::cout << "DLL Loaded: Initializing game module systems!\n";
}

void Game::onUpdate(float deltaTime)
{
}

void Game::onUnload()
{
	std::cout << "DLL Unloaded: Cleaning up game module memory!\n";
}

void Game::release()
{
	delete this; // deletes the object using the DLL's heap
}

// define the factory function that instantiates the Game class
GAME_MAIN_FACTORY(Game)