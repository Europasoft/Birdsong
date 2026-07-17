#pragma once
#include "core/types/Transform.h"

namespace EngineCore
{
	class EngineDevice;
	struct MaterialCreateInfo;
	class Material;
}
namespace WorldSystem
{
	class SectorCoord;
	class Sector;
}

namespace Nodes
{
	using namespace EngineCore;
	using namespace WorldSystem;

	class Node
	{
	public:
		Node() = default;
		virtual ~Node() = default;
		// nodes are not copyable
		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;
		Node(Node&&) = default;

		EngineDevice& getDevice() const;
		void setDevice(EngineCore::EngineDevice& d);
		const Transform& getTransform() const;
		void setTransform(const Transform& t);
		void setSectorCoord(const SectorCoord& s);

		void setMaterial(std::shared_ptr<Material> newMaterial);
		void setMaterial(const MaterialCreateInfo& info);
		std::shared_ptr<Material> getMaterial() const;

	protected:
		friend class Sector;
		EngineDevice* device = nullptr;
		Transform transform;
		std::shared_ptr<Material> material;
		
	};
}