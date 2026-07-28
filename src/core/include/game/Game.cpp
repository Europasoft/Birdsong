#include "core/include/game/Game.h"

namespace EngineInterface
{
	// interface functions called by the engine executable, running in the DLLs memory space
	void DLL_CALL Game::onLoadCall(IEngine* engineItf)
	{
		engine = engineItf; // game code can use this pointer to make calls in the other direction
		onLoad();
	}

	void DLL_CALL Game::onTickCall(double dt)
	{
		tick(dt);
	}

	void DLL_CALL Game::onUnloadCall()
	{
		onUnload();
		release();
	}

	void DLL_CALL Game::release()
	{
		delete this;
	}

}