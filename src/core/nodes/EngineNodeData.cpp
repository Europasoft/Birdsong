#pragma once
#include "core/nodes/EngineNodeData.h"
#include "core/gpu/Device.h"
#include "core/engine/interop/IEngineImpl.h"
#include "core/include/shared/INode.h"
#include "deps/box3d-cpp/include/b3cpp.h"

namespace WorldSystem
{
	EngineNodeData::EngineNodeData(EngineInterface::INode* iNode, EngineCore::EngineDevice& device)
		: iNode(iNode), device(device)
	{}

	EngineNodeData::~EngineNodeData()
	{}

	void EngineNodeData::updateTransformFromGame()
	{
		// BOUNDARY CROSSING: query the game-side node object for transform updates
		if (iNode->getDidTeleport())
		{
			engineTransform = EngineInteropUtil::getNodeTransform(iNode);
			teleported = true;
		}
	}

	b3cpp::Body& EngineNodeData::addPhysicsBody(b3cpp::BodyDef def, b3cpp::World& w)
	{
		physicsBody = w.createBody(def);
		return *physicsBody;
	}

	b3cpp::Body& EngineNodeData::getPhysicsBody()
	{
		return *physicsBody;
	}

}