#include "core/include/game/Node.h"
#include "shared/IEngine.h"
#include "shared/BoundaryUtils.h"

#include <cassert>

namespace EngineInterface
{
	Node::Node(IEngine* enginePtr, size_t sizeOfDerived)
		: engine(enginePtr), sizeOfThis(sizeOfDerived), teleported(true)
	{
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
		BoundaryUtils::packTransform(transform, buffer); // send to engine
	}

	void Node::setTransform(const uint8_t* buffer)
	{
		BoundaryUtils::unpackTransform(buffer, transform); // retrieve from engine
		teleported = false;
	}

	bool Node::getDidTeleport() const
	{
		return teleported;
	}

	void Node::setTransform(const Transform& newTransform)
	{
		transform = newTransform;
		teleported = true;
	}

	void Node::setTranslation(const Vec newTranslation)
	{
		transform.translation = newTranslation;
		teleported = true;
	}

	const Transform& Node::getTransform() const
	{
		return transform;
	}
}