#pragma once

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <filesystem>

namespace EngineCore
{
	struct MeshBuilder;
}

namespace AssetSystem
{
	class Asset;
	class Collection;

	enum class AssetFileFormat : uint32_t
	{
		NONE = 0,
		GLTF_GLB = 1,
		OBJ = 2
	};

	struct ImportSourceInfo
	{
		std::filesystem::path originalFilePath;
		AssetFileFormat fileFormat = AssetFileFormat::NONE;
		Collection* importCollection = nullptr; // if this asset or collection was imported as part of another collection
	};

	class Collection
	{
	public:
		Collection(std::string_view name);
		~Collection();

		std::string name;
		std::vector<std::unique_ptr<Collection>> nested;
		std::vector<std::unique_ptr<Asset>> assets;
		Collection* parent = nullptr;

		// path and format are only relevant if this collection was imported from a file
		ImportSourceInfo importSourceInfo = {};

	public:
		bool reimport();
		bool addCollection(std::string_view name);
		bool addCollectionFromFile(std::filesystem::path path);

	};

	class Asset
	{
	public:
		Asset(std::string_view name);
		virtual ~Asset();

		std::string name;
		// path and format are only relevant if this asset was directly imported, otherwise imported as part of a collection
		ImportSourceInfo importSourceInfo = {};

	public:
		bool reimport();

	};

	class MeshAsset : public Asset
	{
	public:
		using Asset::Asset;

		std::shared_ptr<EngineCore::MeshBuilder> geometry = nullptr;
	};

}