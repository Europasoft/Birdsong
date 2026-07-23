#include "core/world/NodeContainer.h"

#include <cassert>
#include <utility>
#include <ranges>

namespace WorldSystem
{
	using INode = ::EngineInterface::INode;

    void NodeContainer::add(INode* iNode, std::unique_ptr<EngineNodeData>&& eNode)
	{
		assert(iNode);
		if (!iNode) return;

		// insert into map (if not already present)
		auto [it, inserted] = nodes.emplace(iNode,
			NodeMapEntry
			{
				.eNode = std::move(eNode),
				.index = nodesVector.size() // store the index where node will land in the vector
			});

		assert(inserted && "failed to add new node to container");
		if (not inserted) return;

		// add to vector
		nodesVector.push_back(
			NodeVectorEntry
			{
				.iNode = iNode,
				.eNode = it->second.eNode.get() 
			});
	}

	void NodeContainer::remove(INode* iNode)
	{
		assert(iNode);
		if (!iNode) return;

		NodeMapIterator it = nodes.find(iNode);
		if (it == nodes.end()) return;

		const size_t indexToRemove = it->second.index; // get the vector index to swap out

		// swap-and-pop if it's not already the back element
		assert(nodesVector.size() > 0 && "tried to remove node from empty container, race condition or implementation mistake");
		if (indexToRemove != (nodesVector.size() - 1))
		{
			NodeVectorEntry tail = nodesVector.back();
			// overwrite target slot with tail element
			nodesVector[indexToRemove] = tail;
			// update the moved node's map entry with its new index
			auto tailIt = nodes.find(tail.iNode);
			assert(tailIt != nodes.end());
			tailIt->second.index = indexToRemove;
			//nodes[tail.iNode].index = indexToRemove; (simpler but slower)
		}

		// remove the last vector element
		nodesVector.pop_back();
		// remove from map
		nodes.erase(it);
	}

	bool NodeContainer::exists(EngineInterface::INode* iNode) const
	{
		NodeMapIteratorConst it = nodes.find(iNode);
		return (it != nodes.end());
	}

	


}
