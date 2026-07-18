#pragma once

#include <string_view>
#include <memory>
#include <vector>

namespace EngineInterface 
{
	class IGame;
}

namespace EngineCore
{
	class LoadedDLL; // forward declare to avoid DLL handle macros in header (handle type depends on platform)
	using CreateGameFunc = EngineInterface::IGame* (*)(); // function pointer signature matching the exported factory function in the game DLL

	class GameLoader
	{
	public:
		GameLoader();
		~GameLoader();
		void loadDll(std::string_view fileName);

	private:
		std::vector<std::unique_ptr<LoadedDLL>> dlls;
	};
}
