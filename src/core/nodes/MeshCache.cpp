#include "core/nodes/MeshCache.h"
#include "core/engine/MeshData.h"
#include "core/types/CommonTypes.h"

namespace EngineCore
{
	std::shared_ptr<MeshBuilder> MeshCache::getMeshBuilder(const std::string& filePath, bool load)
	{
		auto it = cache.find(filePath);
		if (it != cache.end())
		{
			return it->second; // return existing mesh
		}

		// load new mesh from disk
		auto mb = std::make_shared<MeshBuilder>();
		if (load) mb->loadFromFile(makePath(filePath));

		cache[filePath] = mb;
		return mb;
	}

	void MeshCache::clearUnused()
	{
		for (auto it = cache.begin(); it != cache.end(); )
		{
			if (it->second.use_count() == 1)
			{
				it = cache.erase(it); // only held by cache
			}
			else
			{
				++it;
			}
		}
	}

	void MeshCache::clearAll()
	{
		cache.clear();
	}
}