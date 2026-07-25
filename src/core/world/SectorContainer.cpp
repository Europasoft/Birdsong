#include "core/world/SectorContainer.h"
#include "core/world/Sector.h"

namespace WorldSystem
{
	Sector* SectorContainer::getOrCreateSector(const SectorCoord& coord, const ESectorLookup& mode)
	{
		auto it = sectors.find(coord);
		if (it != sectors.end())
		{
			// found
			return it->second.get();
		}
		if (mode == ESectorLookup::FIND_OR_CREATE)
		{
			// did not exist, create sector on demand
			auto& sectorPtr = sectors[coord];
			sectorPtr = std::make_unique<Sector>(coord);
			return sectorPtr.get();
		}
		return nullptr;
	}
}