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
namespace b3cpp
{
	class World;
	class Body;
	struct BodyDef;
}

namespace WorldSystem
{
	class Mesh;

	// contains the engine-side context for a node
	class EngineNodeData
	{
	public:
		EngineNodeData(EngineInterface::INode* iNode, EngineCore::EngineDevice& device);
		~EngineNodeData();
		// not copyable
		EngineNodeData(const EngineNodeData&) = delete;
		EngineNodeData& operator=(const EngineNodeData&) = delete;

		// gets the transform from the game-side node
		void updateTransformFromGame();

		b3cpp::Body& addPhysicsBody(b3cpp::BodyDef def, b3cpp::World& w);
		b3cpp::Body& getPhysicsBody();

	public:
		EngineInterface::INode* iNode = nullptr;

		// mesh data may or may not be available, depending on how the node was configured in game code
		std::unique_ptr<Mesh> mesh = nullptr;

		// this is updated when the game has changed the node's transform
		Transform engineTransform;
		bool teleported = false;

		std::unique_ptr<b3cpp::Body> physicsBody;

	protected:
		EngineCore::EngineDevice& device;
	};

}