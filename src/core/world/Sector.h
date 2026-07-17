#pragma once
#include "core/nodes/Node.h"
#include "core/types/CommonTypes.h"
#include "deps/box3d-cpp/include/b3cpp.h"

#include <stdint.h>
#include <vector>
#include <memory>

namespace EngineCore
{
	class EngineDevice;
	class DebugDrawer;
	class MeshDrawer;
}

namespace Nodes
{
	class MeshNode;
}

namespace WorldSystem
{
	class Scene;

	class Sector
	{
	public:
		Sector(const SectorCoord& coord);
		static constexpr uint32_t SECTOR_SIZE = 10000;

		std::vector<Nodes::MeshNode*> getMeshNodes() const;

	protected:
		friend class Scene;
		friend class EngineCore::DebugDrawer;
		friend class EngineCore::MeshDrawer;

		SectorCoord coordinates;
		std::vector<std::unique_ptr<Nodes::Node>> nodes;
		bool isSectorCulled = false;

		std::unique_ptr<b3cpp::World> physicsWorld;

		template <typename T>
		requires std::derived_from<T, Nodes::Node>
		T& createNode(EngineCore::EngineDevice& device)
		{
			nodes.push_back(std::unique_ptr<T>(new T()));
			nodes.back()->device = &device;
			nodes.back()->setSectorCoord(coordinates);
			return *static_cast<T*>(nodes.back().get());
		}

	};

	// transforms a position from its native local sector frame into a 3D coordinate relative to a target reference sector's origin
	Vec calculateRelative(Vec subjectLocalCoords, SectorCoord subjectSector, SectorCoord referenceSector);

}