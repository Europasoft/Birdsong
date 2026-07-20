#pragma once

#include <string_view>
#include <memory>
#include <vector>

namespace EngineInterface 
{
	class IGame;
	class IEngine;
}

namespace EngineCore
{
	class LoadedDLL; // forward declare to avoid DLL handle macros in header (handle type depends on platform)
	class IEngineImpl;
	class EngineApplication;

	class GameLoader
	{
	public:
		GameLoader(EngineApplication* engine);
		~GameLoader();
		void loadDll(std::string_view fileName);
		void unloadAll();
		void tick(double dt);

	private:
		std::vector<std::unique_ptr<LoadedDLL>> dlls;
		std::unique_ptr<IEngineImpl> engineItf;
	};
}
