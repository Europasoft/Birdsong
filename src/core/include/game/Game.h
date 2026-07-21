#pragma once
#include "core/include/shared/IGame.h"
#include "core/include/shared/BoundaryUtils.h"
#include "core/include/shared/IEngine.h"

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
		// these can be overridden by a user-created game class
		virtual void onLoad() {};
		virtual void tick(double dt) {};
		virtual void onUnload() {};

	protected:
		IEngine* engine = nullptr;

	public:
		// interface functions called by the engine executable, running in the DLLs memory space
		void DLL_CALL onLoadCall(IEngine* engineItf) final override;

		void DLL_CALL onTickCall(double dt) final override;

		void DLL_CALL onUnloadCall() final override;

		void DLL_CALL release() final override;
	};
}