#pragma once
#include "core/include/shared/IGame.h"
#include "core/include/shared/BoundaryUtils.h"

// macro used in DLL code to define the factory function which creates an instance of the game class
#if defined(_WIN32) || defined(_WIN64)
#define GAME_MAIN_FACTORY(GameClass) extern "C" __declspec(dllexport) EngineInterface::IGame* IG_FACTORY() { return new GameClass(); }
#else
	// Linux version
#define GAME_MAIN_FACTORY(GameClass) extern "C" __attribute__((visibility("default"))) EngineInterface::IGame* IG_FACTORY() { return new GameClass(); }
#endif


namespace EngineInterface
{
	class Game : public IGame
	{
	protected:
		// these can be overridden by game classes
		virtual void onLoad() {};
		virtual void tick(float dt) {};
		virtual void onUnload() {};

	public:
		// interface functions called by the engine executable, running in the DLLs memory space
		void DLL_CALL onLoadCall() final override
		{
			onLoad();
		}

		void DLL_CALL onTickCall(float dt) final override
		{
			tick(dt);
		}

		void DLL_CALL onUnloadCall() final override
		{
			onUnload();
		}

		void DLL_CALL release() final override
		{
			delete this;
		}
	};
}