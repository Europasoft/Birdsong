#include "core/include/game/Game.h"

namespace EngineInterface
{
	// interface functions called by the engine executable, running in the DLLs memory space
	void Game::onLoadCall(IEngine* engineItf)
	{
		engine = engineItf; // game code can use this pointer to make calls in the other direction
		onLoad();
	}

	void Game::onTickCall(double dt)
	{
		tick(dt);
	}

	void Game::onUnloadCall()
	{
		onUnload();
		release();
	}

	void Game::release()
	{
		delete this;
	}

}