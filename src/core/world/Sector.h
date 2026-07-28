#pragma once
#include "core/types/CommonTypes.h"
#include "core/include/shared/Transform.h"

#include <stdint.h>
#include <vector>
#include <memory>

namespace EngineCore
{
	class EngineDevice;
	class DebugDrawer;
	class MeshDrawer;
}
namespace b3cpp
{
	class World;
}

namespace WorldSystem
{
	class Scene;
	class NodeContainer;

	enum class ESectorLookup : int32_t
	{
		FIND_EXISTING = 0,
		FIND_OR_CREATE = 1,
	};

	class Sector
	{
	public:
		Sector(const SectorCoord& coord);
		~Sector();
		static constexpr uint32_t SECTOR_SIZE = 10000;

		NodeContainer& nodes() const;

		b3cpp::World& getPhysicsWorld() const;
		void physicsTick();
		void gamePostPhysicsUpdate();

	protected:
		friend class Scene;
		friend class EngineCore::DebugDrawer;
		friend class EngineCore::MeshDrawer;

		SectorCoord coordinates;
		std::unique_ptr<NodeContainer> nodesContainer;
		bool isSectorCulled = false;

		std::unique_ptr<b3cpp::World> physicsWorld;

	};

	// transforms a position from its native local sector frame into a 3D coordinate relative to a target reference sector's origin
	Vec calculateRelative(Vec subjectLocalCoords, SectorCoord subjectSector, SectorCoord referenceSector);

}