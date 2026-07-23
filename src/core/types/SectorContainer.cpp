#include "core/types/SectorContainer.h"
#include "core/world/Sector.h"

namespace WorldSystem
{
	Sector* SectorContainer::getSector(const SectorCoord& coord) const
	{
		auto it = sectors.find(coord);
		if (it != sectors.end())
		{
			return it->second.get();
		}
		return nullptr; // sector isn't loaded yet
	}

	Sector& SectorContainer::getOrCreateSector(const SectorCoord& coord)
	{
		auto& sectorPtr = sectors[coord];
		if (!sectorPtr)
		{
			sectorPtr = std::make_unique<Sector>(coord);
		}
		return *sectorPtr;
	}

}