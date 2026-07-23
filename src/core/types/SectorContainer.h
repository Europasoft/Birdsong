#pragma once
#include "core/include/shared/Transform.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace WorldSystem
{
	class Sector;

	// custom hash function for SectorCoord - this does not uniquely identify every possible sector, but it speeds up hash map searches
	struct SectorCoordHash
	{
		std::size_t operator()(const WorldSystem::SectorCoord& c) const noexcept
		{
			// 64-bit integer hash mixer
			std::size_t h = 0;
			auto hash_combine = [&h](WorldSystem::SectorInt val)
				{
					std::size_t k = static_cast<std::size_t>(val);
					// magic constant derived from the golden ratio to distribute bits evenly
					k ^= k >> 30;
					k *= 0xbf58476d1ce4e5b9ULL;
					k ^= k >> 27;
					k *= 0x94d049bb133111ebULL;
					k ^= k >> 31;
					h ^= k + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
				};

			hash_combine(c.x);
			hash_combine(c.y);
			hash_combine(c.z);
			return h;
		}
	};

	class SectorContainer
	{
	public:
		SectorContainer() = default;
		~SectorContainer() = default;

	public:
		// get an existing sector (O(1) average lookup)
		Sector* getSector(const SectorCoord& coord) const;

		// get or create on demand
		Sector& getOrCreateSector(const SectorCoord& coord);

	protected:
		using SectorMap = std::unordered_map<WorldSystem::SectorCoord, std::unique_ptr<Sector>, SectorCoordHash>;
		SectorMap sectors;
	};
}