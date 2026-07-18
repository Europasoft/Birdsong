#include "GameMain.h"
#include "IGameMacros.h"

void Game::onLoad()
{
	std::cout << "Game DLL onLoad called\n";
}

void Game::onUpdate(float deltaTime)
{
	std::cout << "Game DLL onUpdate called\n";
}

void Game::onUnload()
{
	std::cout << "Game DLL onUnload called\n";
}

void Game::release()
{
	delete this; // deletes the object using the DLL's heap
}

// define the factory function that instantiates the Game class
GAME_MAIN_FACTORY(Game)