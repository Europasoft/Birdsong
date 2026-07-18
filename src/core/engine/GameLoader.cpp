#include "core/engine/GameLoader.h"
// engine public headers
#include "core/include/IGame.h" 
#include "core/include/IGameMacros.h"

#include <stdexcept>
#include <cassert>
#include <iostream>
#include <string>

// platform specifics
#if defined(_WIN32)
	#include <windows.h>
	static constexpr auto dllPathPrefix = "";
	static constexpr auto dllPathSuffix = ".dll";
	using DLLHandle = HMODULE;
	#define LOAD_LIBRARY(path) LoadLibraryA(path)
	#define GET_FUNCTION(handle, name) GetProcAddress(handle, name)
	#define FREE_LIBRARY(handle) FreeLibrary(handle)
#else
	#include <dlfcn.h>
	static constexpr auto dllPathPrefix = "./";
	static constexpr auto dllPathSuffix = ".so";
	using DLLHandle = void*;
	#define LOAD_LIBRARY(path) dlopen(path, RTLD_NOW)
	#define GET_FUNCTION(handle, name) dlsym(handle, name)
	#define FREE_LIBRARY(handle) dlclose(handle)
#endif

namespace EngineCore
{
	// wrapper around a platform-specific DLLHandle
	class LoadedDLL
	{
	private:
		DLLHandle handle = nullptr;
		EngineInterface::IGame* game = nullptr;

	public:
		LoadedDLL(std::string_view path);
		~LoadedDLL();

		void load(std::string_view path);
		void unload();
		bool valid() const;
		EngineInterface::IGame& getGame() const;
	};

	LoadedDLL::LoadedDLL(std::string_view path)
	{
		load(path);
	}

	LoadedDLL::~LoadedDLL()
	{
		unload();
	}

	void LoadedDLL::load(std::string_view path)
	{
		// load the binary into the executable's address space
		handle = LOAD_LIBRARY(std::string(path).c_str());
		if (handle == nullptr) return;
		std::cout << "Loaded DLL: '" << path << "'\n";

		// find the address of the game class factory C-function in the DLL
		CreateGameFunc createGame = (CreateGameFunc)GET_FUNCTION(handle, igameFactoryNameString);
		if (createGame)
		{
			// BOUNDARY CROSSING: instantiate the game object and call onLoad
			game = createGame();
			game->onLoad();
		}
		else
		{
			throw std::runtime_error("Failed to find factory function in DLL " + std::string(path));
		}
	}

	void LoadedDLL::unload()
	{
		// free the game object
		if (game)
		{
			game->release();
			game = nullptr;
		}
		// free the DLL binary
		if (handle)
		{
			FREE_LIBRARY(handle);
			handle = nullptr;
		}
	}

	bool LoadedDLL::valid() const
	{
		return (handle != nullptr);
	}

	EngineInterface::IGame& LoadedDLL::getGame() const
	{
		assert(game && "DLL game object was not instantiated");
		return *game;
	}


	GameLoader::GameLoader() {}
	GameLoader::~GameLoader() {}

	void GameLoader::loadDll(std::string_view fileName)
	{
		std::string path = dllPathPrefix;
		path.append(fileName);
		path.append(dllPathSuffix);
		
		dlls.push_back(std::make_unique<LoadedDLL>(path));
		LoadedDLL& dll = *dlls.back();

		if (not dll.valid())
		{
			throw std::runtime_error("Failed to load DLL " + path);
		}

		// BOUNDARY CROSSING: test update function
		dll.getGame().onUpdate(0.016f);

	}
}
