#pragma once
#include "core/nodes/EMesh.h"
#include "core/include/shared/Transform.h"

#include <memory>

namespace EngineCore
{
	class EngineDevice;
}
namespace EngineInterface
{
	class INode;
}

namespace WorldSystem
{
	class Mesh;

	// contains the extra engine-side context for a node, this is the base class
	class EngineNodeData
	{
	public:
		EngineNodeData(EngineInterface::INode* iNode, EngineCore::EngineDevice& device);
		~EngineNodeData();
		// not copyable
		EngineNodeData(const EngineNodeData&) = delete;
		EngineNodeData& operator=(const EngineNodeData&) = delete;

		// mesh data may or may not be available, depending on how the node was configured in game code
		std::unique_ptr<Mesh> mesh = nullptr;
		// this is updated when the game has changed the node's transform
		Transform engineTransform;

		// gets the transform from the game-side node
		void updateTransformFromGame();

	protected:
		EngineCore::EngineDevice& device;
		EngineInterface::INode* iNode = nullptr;
	};

}