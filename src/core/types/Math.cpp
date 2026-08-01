// Copyright 2026 Simon Liimatainen. All rights reserved.
#include "core/types/Math.h"
#include "core/world/Sector.h"

#include <cmath>
#include <numeric>

namespace Math
{
	using namespace WorldSystem;

	static constexpr double SECTOR_SIZE_D = WorldSystem::Sector::SECTOR_SIZE;

	double calculateRelativeCoord(double offsetA, WorldSystem::SectorInt sectorA, double offsetB, WorldSystem::SectorInt sectorB)
	{
		// example: to get position of object relative to player's sector: (0, playerSectorX, objectOffsetX, objectSectorX)
		// example: to get position of B relative to A: (AX, ASectorX, BX, BSectorX)
		// example: to get distance between A and B: calculateRelativePos ^ 2 for each axis, sum the results, then sqrt
		// example: to get direction from A to B: get distance, then divide each axis result of calculateRelativePos by the distance
		// example: to get direction from B to A: swap the order of inputs
		const SectorInt sectorDelta = sectorB - sectorA; // distance measured in sectors
		const double distanceDelta = static_cast<double>(sectorDelta) * SECTOR_SIZE_D; // actual world distance
		return (offsetB - offsetA) + distanceDelta;
	}

	Vec64 calculateRelativeCoordsXYZ(const Transform& a, const Transform& b)
	{
		return Vec64
		{
			calculateRelativeCoord(a.translation.x, a.sector.x, b.translation.x, b.sector.x),
			calculateRelativeCoord(a.translation.y, a.sector.y, b.translation.y, b.sector.y),
			calculateRelativeCoord(a.translation.z, a.sector.z, b.translation.z, b.sector.z),
		};
	}

	Vec calculateRelativePositionForRendering(const Transform& t, const WorldSystem::SectorCoord& cameraSector)
	{
		// rebase the position so it becomes relative to the center of the camera's sector
		return Vec
		{
			static_cast<float>(calculateRelativeCoord(0.0, cameraSector.x, t.translation.x, t.sector.x)),
			static_cast<float>(calculateRelativeCoord(0.0, cameraSector.y, t.translation.y, t.sector.y)),
			static_cast<float>(calculateRelativeCoord(0.0, cameraSector.z, t.translation.z, t.sector.z))
		};
	}

	double calculateDistance(const Transform& a, const Transform& b)
	{
		return calculateRelativeCoordsXYZ(a, b).getLength();
	}

	double calculateDistanceToSectorCenter(const Transform& start, const WorldSystem::SectorCoord& targetSector)
	{
		Transform b = {};
		b.sector = targetSector;
		return calculateRelativeCoordsXYZ(start, b).getLength();
	}

	Vec calculateDirection(const Transform& a, const Transform& b)
	{
		const Vec64 v = calculateRelativeCoordsXYZ(a, b);
		const double d = v.getLength();
		if (d < 1e-12) // guard against division by zero
		{ 
			return Vec(0);
		}
		return Vec
		{
			static_cast<float>(v.x / d),
			static_cast<float>(v.y / d),
			static_cast<float>(v.z / d)
		};
	}

	Vec calculateDirectionToSectorCenter(const Transform& start, const WorldSystem::SectorCoord& targetSector)
	{
		Transform b = {};
		b.sector = targetSector;
		return calculateDirection(start, b);
	}

}