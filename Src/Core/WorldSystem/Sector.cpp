#include "Core/WorldSystem/Sector.h"
#include "Core/WorldSystem/World.h"
#include "Core/Primitive.h"
#include <iostream>
namespace WorldSystem
{
	SectorCoord::SectorCoord() : x{ 0 }, y{ 0 }, z{ 0 } {};
	SectorCoord::SectorCoord(SectorInt x, SectorInt y, SectorInt z) : x{ x }, y{ y }, z{ z } {};

	Sector::Sector(const SectorCoord& coord)
		: coordinates{ coord }
	{
		physicsWorld = std::make_unique<b3cpp::World>();

		b3cpp::BodyDef bodyDef;
		bodyDef.type = b3cpp::EBodyType::DynamicBody;
		b3cpp::Body b = physicsWorld->createBody(bodyDef);

		b3cpp::SphereShape& s = b.createShape<b3cpp::SphereShape>();
		b3cpp::ShapeDef shapeDef;
		
		s.activate(shapeDef);
	}

	Vec calculateRelative(Vec subjectLocalCoords, SectorCoord subjectSector, SectorCoord referenceSector)
	{
		// very specific order of operations, to avoid floating point inaccuracy
		auto calculateRelativeCoord = [&](float localCoord, SectorInt localSectorCoord, SectorInt referenceSectorCoord)
		{
			const SectorInt sectorDelta = localSectorCoord - referenceSectorCoord; // distance measured in sectors
			const double distanceDelta = static_cast<double>(sectorDelta) * static_cast<double>(Scene::SECTOR_SIZE); // actual world distance
			const double relative = static_cast<double>(localCoord) + distanceDelta;
			return static_cast<float>(relative);
		};

		return Vec
			{
				calculateRelativeCoord(subjectLocalCoords.x, subjectSector.x, referenceSector.x),
				calculateRelativeCoord(subjectLocalCoords.y, subjectSector.y, referenceSector.y),
				calculateRelativeCoord(subjectLocalCoords.z, subjectSector.z, referenceSector.z)
			};
	}


}

