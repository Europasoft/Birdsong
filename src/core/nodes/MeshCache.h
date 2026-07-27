#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>

namespace EngineCore
{
	struct MeshBuilder;

	class MeshCache
	{
	public:
		MeshCache() = default;
		~MeshCache() = default;
		MeshCache(const MeshCache&) = delete;
		MeshCache& operator=(const MeshCache&) = delete;

		// fetch an existing mesh or load it from file if missing
		std::shared_ptr<MeshBuilder> getMeshBuilder(const std::string& filePath, bool load = true);
		
		// clear cached meshes that aren't being used anywhere else
		void clearUnused();

		void clearAll();

	private:
		std::unordered_map<std::string, std::shared_ptr<MeshBuilder>> cache;
	};
}

