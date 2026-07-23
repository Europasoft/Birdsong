#include "core/world/Sector.h"
#include "core/world/NodeContainer.h"
#include "core/world/World.h"
#include "core/nodes/Node.h"
#include "core/nodes/MeshNode.h"

#include "deps/box3d-cpp/include/b3cpp.h"

#include <iostream>

namespace WorldSystem
{
	Sector::Sector(const SectorCoord& coord)
		: coordinates{ coord }
	{
		nodes = std::make_unique<NodeContainer>();

		b3cpp::WorldDef wd;
		wd.gravity = { 0, 0, -0.1 };
		physicsWorld = std::make_unique<b3cpp::World>(wd);
		assert(physicsWorld->isIdValid());
	}

	Sector::~Sector() 
	{}

	Vec calculateRelative(Vec subjectLocalCoords, SectorCoord subjectSector, SectorCoord referenceSector)
	{
		// very specific order of operations, to avoid floating point inaccuracy
		auto calculateRelativeCoord = [&](float localCoord, SectorInt localSectorCoord, SectorInt referenceSectorCoord)
		{
			const SectorInt sectorDelta = localSectorCoord - referenceSectorCoord; // distance measured in sectors
			const double distanceDelta = static_cast<double>(sectorDelta) * static_cast<double>(Sector::SECTOR_SIZE); // actual world distance
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

	NodeContainer& Sector::getNodes() const
	{
		return *nodes.get();
	}

	b3cpp::World& Sector::getPhysicsWorld() const
	{
		return *physicsWorld.get();
	}

	void Sector::physicsTick()
	{
		static bool didExplode = false;//TMP
		if (not physicsWorld) return;

		// TODO: ASAP: make this work with the new system based on EngineNodeData
		//for (Nodes::MeshNode* node : getMeshNodes())
		//{
		//	node->physicsTick();
		//}

		physicsWorld->step();

		//TMP
		if (!didExplode)
		{
			b3cpp::ExplosionDef x;
			x.falloff = 10000;
			x.radius = 1000;
			x.position = { 0, 400, -200 };
			x.impulsePerArea = 100000 * 100;
			physicsWorld->explode(x);
			didExplode = 1;
		}
	}


}

