#pragma once
#include <stdint.h>
#include <vector>
#include <memory>

#include "Core/Types/CommonTypes.h"
#include "Core/Dependencies/box3d-cpp/include/b3cpp.h"

namespace EngineCore
{
	class Primitive;
}

namespace WorldSystem
{
	using SectorInt = intmax_t;

	class SectorCoord
	{
	public:
		SectorCoord();
		SectorCoord(SectorInt x, SectorInt y, SectorInt z);
		SectorInt x, y, z;
		bool operator==(const SectorCoord& s) const { return x == s.x && y == s.y && z == s.z; } // ==
		bool operator!=(const SectorCoord& s) const { return !(s == *this); } // !=
		SectorCoord operator+(const SectorCoord& s) const { return SectorCoord{ x + s.x, y + s.y, z + s.z }; } // +
		SectorCoord operator-(const SectorCoord& s) const { return SectorCoord{ x - s.x, y - s.y, z - s.z }; } // -
		SectorCoord operator+=(const SectorCoord& s) { *this = s + *this; return *this; } // +=
		SectorCoord operator-=(const SectorCoord& s) { *this = s - *this; return *this; } // -=
	};

	class Sector
	{
	public:
		Sector(const SectorCoord& coord);

		SectorCoord coordinates;
		std::vector<std::unique_ptr<EngineCore::Primitive>> primitives;
		bool isCulled = false;

		std::unique_ptr<b3cpp::World> physicsWorld;

	};

	// transforms a position from its native local sector frame into a 3D coordinate relative to a target reference sector's origin
	Vec calculateRelative(Vec subjectLocalCoords, SectorCoord subjectSector, SectorCoord referenceSector);

}