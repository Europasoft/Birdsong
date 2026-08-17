#include "core/asset/Collection.h"

#include <stdexcept>
#include <cassert>

namespace AssetSystem
{
	Collection::Collection(std::string_view name)
		: name(name)
	{
	}

	Collection::~Collection() = default;

	// reimports an entire collection from a file, if applicable
	bool Collection::reimport()
	{
		if (importSourceInfo.originalFilePath.empty()) return false;
		assert(importSourceInfo.fileFormat != AssetFileFormat::NONE && importSourceInfo.importCollection == nullptr);

		// TODO
		return false;
	}

	bool Collection::addCollection(std::string_view name)
	{
		if (importSourceInfo.importCollection) return false; // can't manually add to a hierarchy loaded from file
		
		nested.push_back(std::make_unique<Collection>(name));
		return true;
	}

	bool Collection::addCollectionFromFile(std::filesystem::path path)
	{
		return false;
	}

	Asset::Asset(std::string_view name)
		: name(name)
	{
	}

	Asset::~Asset() = default;

	// reimports the whole collection if asset is part of a collection imported from a file, or just this asset if not
	bool Asset::reimport()
	{
		if (importSourceInfo.importCollection)
		{
			//assert((importSourceInfo.originalFilePath.empty() && importSourceInfo.fileFormat == AssetFileFormat::NONE)
			//		|| importSourceInfo.importCollection == this);
			importSourceInfo.importCollection->reimport();
		}
		else
		{
			assert(importSourceInfo.fileFormat != AssetFileFormat::NONE && not importSourceInfo.originalFilePath.empty());

			// TODO: reimport just this asset
		}
		return false;
	}
	
}