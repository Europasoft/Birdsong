#include "core/include/game/Node.h"

namespace EngineInterface
{
	// interface functions called by the engine executable, running in the DLLs memory space
	void Node::release()
	{
		delete this;
	}

	void Node::onSpawnCall()
	{
		onSpawn();
	}

	void Node::tickCall(float dt)
	{
		tick(dt);
	}

	void Node::getTransform(uint8_t* buffer) const
	{
		BoundaryUtils::packTransform(transform, buffer);
	}

	void Node::setTransform(const uint8_t* buffer)
	{
		BoundaryUtils::unpackTransform(buffer, transform);
	}
}