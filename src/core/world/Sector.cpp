#include "core/world/Sector.h"
#include "core/world/NodeContainer.h"
#include "core/world/World.h"
#include "core/include/shared/INode.h"
#include "core/engine/interop/IEngineImpl.h"

#include "deps/box3d-cpp/include/b3cpp.h"

#include <iostream>
#include <cassert>

namespace WorldSystem
{
	using INode = EngineInterface::INode;

	Sector::Sector(const SectorCoord& coord)
		: coordinates{ coord }
	{
		nodesContainer = std::make_unique<NodeContainer>();

		b3cpp::WorldDef wd;
		wd.gravity = { 0, 0, -0.1 };
		physicsWorld = std::make_unique<b3cpp::World>(wd);
		assert(physicsWorld->isIdValid());
	}

	Sector::~Sector() 
	{}

	NodeContainer& Sector::nodes() const
	{
		return *nodesContainer.get();
	}

	b3cpp::World& Sector::getPhysicsWorld() const
	{
		return *physicsWorld.get();
	}

	void Sector::physicsTick()
	{
		if (not physicsWorld) return;

		// pull transform updates from game
		for (EngineNodeData* node : nodesContainer->getMeshes())
		{
			node->mesh->prePhysics(*node); 
		}

		physicsWorld->step(); // simulate physics

		// get updated physics transforms
		for (EngineNodeData* node : nodesContainer->getMeshes())
		{
			node->mesh->postPhysics(*node);
		}
	}

	void Sector::gamePostPhysicsUpdate()
	{
		// push updated physics transforms to game, for next game tick
		for (EngineNodeData* data : nodesContainer->getEngineNodeDatas())
		{
			EngineInteropUtil::setNodeTransform(data->iNode, data->engineTransform); // BOUNDARY CROSSING
		}
	}


}

