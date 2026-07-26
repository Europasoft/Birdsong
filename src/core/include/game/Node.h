#pragma once
#include "core/include/shared/INode.h"
#include "core/include/shared/Transform.h"

// GAME-ONLY INCLUDE

namespace EngineInterface
{
	class IEngine;

	class Node : public INode
	{
	public:
		// Nodes are always instantiated with this constructor
		// derived Nodes must declare "using Node::Node;" to inherit it
		Node(IEngine* enginePtr, size_t sizeOfDerived);
		// Nodes are allowed to be deleted by the game, but not through the shared INode* interface
		~Node();
		// Nodes are not movable/copyable, a node stays permanently at the same memory location
		// this ensures consistent pointer address for comparisons
		Node(Node&&) = delete;
		Node& operator=(Node&&) = delete;
		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;

	protected:
		// these can be overridden in derived classes
		virtual void onSpawn() {};
		virtual void tick(float dt) {};
		virtual void onDestroy() {};
		
		IEngine* engine = nullptr;

	public:
		void setTransform(const Transform& newTransform);
		void setTranslation(const Vec newTranslation);
		void setPosition(const Vec newTranslation) { setTranslation(newTranslation); }
		const Transform& getTransform() const;

	private:
		// interface functions called by the engine executable, running in the DLLs memory space
		void DLL_CALL tickCall(float dt) final override;
		void DLL_CALL getTransform(uint8_t* buffer) const final override;
		void DLL_CALL setTransform(const uint8_t* buffer) final override;
		bool DLL_CALL getDidTeleport() const final override;

		Transform transform;
		bool teleported = false;
		size_t sizeOfThis = 0;
	};
}