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

	void DLL_CALL Node::tickCall(float dt)
	{
		tick(dt);
	}

	void DLL_CALL Node::getTransform(uint8_t* buffer) const
	{
		BoundaryUtils::packTransform(transform, buffer); // send to engine
	}

	void DLL_CALL Node::setTransform(const uint8_t* buffer)
	{
		BoundaryUtils::unpackTransform(buffer, transform); // retrieve from engine
		teleported = false;
	}

	bool DLL_CALL Node::getDidTeleport() const
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