#pragma once
#include "core/nodes/EngineNodeData.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <ranges>
#include <concepts>

namespace EngineInterface
{
	class INode;
}

namespace WorldSystem
{
	struct NodeMapEntry
	{
		std::unique_ptr<EngineNodeData> eNode;
		size_t index;
	};

	struct NodeVectorEntry
	{
		EngineInterface::INode* iNode = nullptr;
		EngineNodeData* eNode = nullptr;
	};

	// engine-side container for tracking game nodes (within a specific sector)
	class NodeContainer
	{
	protected:
		// primary INode mappings
		std::unordered_map<EngineInterface::INode*, NodeMapEntry> nodes;
		using NodeMapIterator = std::unordered_map<EngineInterface::INode*, NodeMapEntry>::iterator;
		using NodeMapIteratorConst = std::unordered_map<EngineInterface::INode*, NodeMapEntry>::const_iterator;

		// contiguous vector for fast iteration during ticking/rendering
		std::vector<NodeVectorEntry> nodesVector;

	public:
		NodeContainer() = default;
		~NodeContainer() = default;

		void add(EngineInterface::INode* iNode, std::unique_ptr<EngineNodeData>&& eNode);

		void remove(EngineInterface::INode* iNode);

		bool exists(EngineInterface::INode* iNode) const;

		// returns an view over every INode* in the container
		auto getINodes()
		{
			return nodesVector | std::views::transform([](const auto& entry) -> EngineInterface::INode*
				{
					return entry.iNode;
				});
		}

		// returns an view over every EngineNodeData* matching the provided subclass type
		template <typename T>
			requires std::derived_from<T, EngineNodeData>
		auto getENodes()
		{
			return nodesVector
				| std::views::filter([](const auto& entry)
					{
						return dynamic_cast<T*>(entry.eNode) != nullptr;
					})
				| std::views::transform([](const auto& entry) -> T*
					{
						return static_cast<T*>(entry.eNode);
					});
		}

		// returns an view over every EngineNodeData* matching the provided subclass type - const
		template <typename T>
			requires std::derived_from<T, EngineNodeData>
		auto getENodes() const
		{
			return nodesVector
				| std::views::filter([](const auto& entry)
					{
						return dynamic_cast<const T*>(entry.eNode) != nullptr;
					})
				| std::views::transform([](const auto& entry) -> const T*
					{
						return static_cast<const T*>(entry.eNode);
					});
		}

	

	};
}