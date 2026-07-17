#include "core/world/Sector.h"
#include "core/world/World.h"
#include "core/nodes/Node.h"
#include "core/nodes/MeshNode.h"

#include <iostream>

namespace WorldSystem
{
	SectorCoord::SectorCoord() : x{ 0 }, y{ 0 }, z{ 0 } {};
	SectorCoord::SectorCoord(SectorInt x, SectorInt y, SectorInt z) : x{ x }, y{ y }, z{ z } {};

	Sector::Sector(const SectorCoord& coord)
		: coordinates{ coord }
	{
		auto n = Nodes::getNodesOfType<Nodes::MeshNode>(nodes);

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

	std::vector<Nodes::MeshNode*> Sector::getMeshNodes() const
	{
		std::vector<Nodes::MeshNode*> meshNodes;
		for (const auto& mesh : Nodes::getNodesOfType<Nodes::MeshNode>(nodes)) 
		{
			meshNodes.push_back(const_cast<Nodes::MeshNode*>(&mesh));
		}
		return meshNodes;
	}


}

