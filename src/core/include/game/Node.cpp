#include "core/include/game/Node.h"
#include "shared/IEngine.h"
#include <cassert>

namespace EngineInterface
{
	Node::Node(IEngine* enginePtr)
	{
		engine = enginePtr;
		engine->registerNode(this);
		onSpawn();
	}

    Node::~Node()
    {
		onDestroy();
		engine->unregisterNode(this);
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