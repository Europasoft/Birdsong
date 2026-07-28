#pragma once
#include "shared/IGame.h"
#include "shared/BoundaryUtils.h"
#include "shared/IEngine.h"
#include "game/Node.h"
#include "game/CommonGameFunctions.h"

// macro used in DLL code to define the factory function which creates an instance of the game class
#if defined(_WIN32) || defined(_WIN64)
#define GAME_MAIN_FACTORY(GameClass) extern "C" __declspec(dllexport) EngineInterface::IGame* IG_FACTORY() { return new GameClass(); }
#else
	// Linux version
#define GAME_MAIN_FACTORY(GameClass) extern "C" __attribute__((visibility("default"))) EngineInterface::IGame* IG_FACTORY() { return new GameClass(); }
#endif

#include <memory>
#include <vector>

// GAME-ONLY INCLUDE

namespace EngineInterface
{
	class Game : public IGame, public CommonGameFunctions
	{
	protected:
		// these can be overridden by a user-created game class
		virtual void onLoad() {};
		virtual void tick(double dt) {};
		virtual void onUnload() {};

	protected:
		IEngine* engine = nullptr;

	public:
		template <typename T, typename... Args>
			requires std::derived_from<T, Node>
		std::unique_ptr<T> spawnNode()
		{
			std::unique_ptr<T> node = std::make_unique<T>(engine, sizeof(T));
			return node; // trusting the developer to keep this around as long as they need it
		}

	private:
		// interface functions called by the engine executable, running in the DLLs memory space
		void DLL_CALL onLoadCall(IEngine* engineItf) final override;
		void DLL_CALL onTickCall(double dt) final override;
		void DLL_CALL onUnloadCall() final override;
		void DLL_CALL release() final override;

	};
}