// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once
#include "core/types/CommonTypes.h"
#include "core/include/shared/Transform.h"

#include <stdint.h>
#include <cmath>
#include <numeric>

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

	// returns multiple of x that is closest to v
	template<typename T>
	T closestMultiple(T v, const T& x)
	{
		if (x > v)
		{
			return x;
		}
		v = v + (x / 2);
		v = v - fmod(v, x);
		return v;
	}

	// returns multiple of m that is closest to but >= v
	template<typename T>
	T roundUpToClosestMultiple(const T& v, const T& m)
	{
		if (m == 0)
		{
			return v;
		}
		const T remainder = v % m;
		if (remainder == 0)
		{
			return v;
		}
		return v + m - remainder;
	}
}
