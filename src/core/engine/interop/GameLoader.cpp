#include "core/engine/interop/GameLoader.h"
#include "core/engine/interop/IEngineImpl.h"
#include "core/engine/Engine.h"
// engine public headers
#include "core/include/shared/BoundaryUtils.h"
#include "core/include/shared/IGame.h"

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
	using CreateGameFunc = EngineInterface::IGame* (*)(); // function pointer signature matching the exported factory function in the game DLL

	// wrapper around a platform-specific DLLHandle
	class LoadedDLL
	{
	private:
		DLLHandle handle = nullptr;
	public:
		std::string filePath;
		EngineInterface::IGame* game = nullptr;

	public:
		LoadedDLL(std::string_view path, IEngineImpl* engineItf);
		~LoadedDLL();

		void load(std::string_view path, IEngineImpl* engineItf);
		void unload();
		bool valid() const;
	};

	LoadedDLL::LoadedDLL(std::string_view path, IEngineImpl* engineItf)
	{
		load(path, engineItf);
	}

	LoadedDLL::~LoadedDLL()
	{
		unload();
	}

	void LoadedDLL::load(std::string_view path, IEngineImpl* engineItf)
	{
		// load the binary into the executable's address space
		handle = LOAD_LIBRARY(std::string(path).c_str());
		if (!handle) return;
		filePath = path;
		std::cout << "Loaded DLL '" << filePath << "'\n";

		// find the address of the game class factory C-function in the DLL
		CreateGameFunc createGame = (CreateGameFunc)GET_FUNCTION(handle, igameFactoryNameString);
		if (createGame)
		{
			// BOUNDARY CROSSING: instantiate the game object and call onLoad
			game = createGame();
			game->onLoadCall(engineItf);
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
			// BOUNDARY CROSSING: calls cleanup logic inside the DLL, then deletes the object
			game->onUnloadCall();
			game = nullptr;
		}
		// free the DLL binary
		if (handle)
		{
			FREE_LIBRARY(handle);
			handle = nullptr;
			std::cout << "Unloaded DLL '" << filePath << "'\n";
		}
	}

	bool LoadedDLL::valid() const
	{
		return handle && game;
	}

	GameLoader::GameLoader(EngineApplication* engine)
	{
		// the engine interface object is exposed to the game DLL as an opaque IEngine pointer
		engineItf = std::make_unique<IEngineImpl>();
		engineItf->engine = engine;
	}

	GameLoader::~GameLoader() 
	{
		unloadAll();
	}

	void GameLoader::loadDll(std::string_view fileName)
	{
		std::string path = dllPathPrefix;
		path.append(fileName);
		path.append(dllPathSuffix);

		dlls.push_back(std::make_unique<LoadedDLL>(path, engineItf.get()));
		LoadedDLL& dll = *dlls.back();

		if (not dll.valid())
		{
			dlls.pop_back();
			throw std::runtime_error("Failed to load DLL " + path);
		}
	}

	void GameLoader::unloadAll()
	{
		dlls.clear(); // destructors handle cleanup
	}

	void GameLoader::tick(double dt)
	{
		for (auto& dll : dlls)
		{
			if (dll->valid())
			{
				// BOUNDARY CROSSING: call tick update
				 dll->game->onTickCall(dt);
			}
		}
	}
}
