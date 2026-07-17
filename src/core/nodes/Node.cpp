#include "core/nodes/Node.h"
#include "core/gpu/Device.h"
#include "core/gpu/Material.h"
#include "core/types/Transform.h"

#include <cassert>

namespace Nodes
{
	using namespace EngineCore;

	EngineCore::EngineDevice& Node::getDevice() const
	{
		assert(device != nullptr && "device ptr was null in node");
		return (*device);
	}

	void Node::setDevice(EngineCore::EngineDevice& d)
	{
		assert(device == nullptr && "tried to set device ptr twice on node");
		device = &d;
	}

	const Transform& Node::getTransform() const
	{
		return transform;
	}

    void Node::setTransform(const Transform& t)
    {
		transform = t;
	}

    void Node::setSectorCoord(const WorldSystem::SectorCoord& s)
    {
		transform.sector = s;
	}

	void Node::setMaterial(std::shared_ptr<Material> newMaterial)
	{ 
		material = newMaterial; 
	}

	void Node::setMaterial(const MaterialCreateInfo& info)
	{ 
		material = std::make_shared<Material>(info, *device); 
	}

	std::shared_ptr<Material> Node::getMaterial() const
	{ 
		return material; 
	}

}