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
	class Mesh;

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

		bool contains(EngineInterface::INode* iNode) const;

		const NodeVectorEntry& getEntry(EngineInterface::INode* iNode) const;

		EngineNodeData* getEngineNodeData(EngineInterface::INode* iNode) const;

		// returns an view over every INode* in the container
		auto getINodes()
		{
			return nodesVector | std::views::transform([](const auto& entry) -> EngineInterface::INode*
				{
					return entry.iNode;
				});
		}

		// returns the EngineNodeData for every node that has a Mesh
		std::vector<EngineNodeData*> getMeshes() const;

	};
}