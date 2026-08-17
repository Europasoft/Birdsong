#pragma once

#include <vector>
#include <string>
#include <memory>
#include <array>
#include <cstdint>
#include <filesystem>

namespace AssetSystem
{
	namespace GLB
	{
		struct BufferView;
		struct Accessor;
		struct Mesh;
	}

	class Collection;

	namespace glTF
	{
		std::unique_ptr<Collection> load(const std::filesystem::path& path);
	}

}