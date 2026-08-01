// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once
#include "core/types/CommonTypes.h"
#include "core/include/shared/Transform.h"

#include <stdint.h>

namespace Math
{
	double calculateRelativeCoord(double offsetA, WorldSystem::SectorInt sectorA, double offsetB, WorldSystem::SectorInt sectorB);

	Vec64 calculateRelativeCoordsXYZ(const Transform& a, const Transform& b);

	Vec calculateRelativePositionForRendering(const Transform& t, const WorldSystem::SectorCoord& cameraSector);

	double calculateDistance(const Transform& a, const Transform& b);
	
	Vec calculateDirection(const Transform& a, const Transform& b);

	// returns the absolute distance between any 3D location and any sector origin
	double calculateDistanceToSectorCenter(const Transform& start, const WorldSystem::SectorCoord& targetSector);

	Vec calculateDirectionToSectorCenter(const Transform& start, const WorldSystem::SectorCoord& targetSector);

}
