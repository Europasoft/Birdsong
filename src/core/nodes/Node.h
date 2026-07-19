#pragma once
#include "core/include/shared/Transform.h"

#include <memory>
#include <vector>
#include <ranges>

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

	// utility to filter a list (std::vector) of Nodes to get only objects of a specific subclass
	template <typename T>
		requires std::derived_from<T, Node>
	auto getNodesOfType(const std::vector<std::unique_ptr<Node>>& nodes)
	{
		return nodes
			| std::views::filter([](const auto& nodePtr)
				{
					return dynamic_cast<const T*>(nodePtr.get()) != nullptr;
				})
			| std::views::transform([](const auto& nodePtr) -> T&
				{
					return *static_cast<T*>(nodePtr.get());
				});
	}
}