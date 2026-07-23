#include "core/engine/interop/IEngineImpl.h"
#include "core/engine/Engine.h"
#include "core/world/World.h"
#include "core/world/Scene.h"

#include "core/include/shared/BoundaryUtils.h"
#include "core/include/shared/Transform.h"
#include "core/include/shared/INode.h"

#include <cassert>

namespace EngineCore
{
	IEngineImpl::~IEngineImpl()
	{}

	void IEngineImpl::registerNode(EngineInterface::INode* node)
	{
		const Transform transform = getNodeTransform(node);
		// get the sector this node belongs to, or create a new one
		WorldSystem::Sector& sector = engine->world->getScene().getSector(transform.sector);
		// add node to the engine registry
		sector.getNodes().add(node, std::make_unique<WorldSystem::EngineNodeData_Mesh>());
	}

	void IEngineImpl::unregisterNode(EngineInterface::INode* node)
	{
		const Transform transform = getNodeTransform(node);
		WorldSystem::Sector& sector = engine->world->getScene().getSector(transform.sector);
		// remove node from the engine registry
		assert(sector.getNodes().exists(node) && "cannot unregister node, was never registered to sector");
		sector.getNodes().remove(node);
	}

	void IEngineImpl::getMousePosition(double& x, double& y) const
	{
		const auto mp = engine->window->input.getMousePosition();
		x = mp.x;
		y = mp.y;
	}

	Transform IEngineImpl::getNodeTransform(INode* node)
	{
		// safely get the node's transform from the game instance
		Transform tf{};
		std::vector<uint8_t> buffer(BoundaryUtils::getTransformDataSize(tf));
		// BOUNDARY CROSSING: briefly call back to the game to fill the buffer
		node->getTransform(buffer.data());
		BoundaryUtils::unpackTransform(buffer.data(), tf);
		return tf;
	}


}