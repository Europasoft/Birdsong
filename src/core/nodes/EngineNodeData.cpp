#pragma once
#include "core/nodes/EngineNodeData.h"
#include "core/gpu/Device.h"
#include "core/engine/interop/IEngineImpl.h"

namespace WorldSystem
{
	EngineNodeData::EngineNodeData(EngineInterface::INode* iNode, EngineCore::EngineDevice& device)
		: iNode(iNode), device(device)
	{}

	EngineNodeData::~EngineNodeData()
	{}

	void EngineNodeData::updateTransformFromGame()
	{
		engineTransform = EngineInteropUtil::getNodeTransform(iNode);
	}

}