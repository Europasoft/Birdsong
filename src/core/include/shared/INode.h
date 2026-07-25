#pragma once
#include "core/include/shared/BoundaryUtils.h"

// SHARED INCLUDE

namespace EngineInterface
{
	// functions called by the engine executable
	class INode
	{
	public:
		virtual void DLL_CALL tickCall(float dt) = 0;
		virtual void DLL_CALL getTransform(uint8_t* buffer) const = 0;
		virtual void DLL_CALL setTransform(const uint8_t* buffer) = 0;
		virtual bool DLL_CALL getDidTeleport() const = 0;

	protected:
		// prevent the engine executable from deleting nodes created in DLL memory
		// the actual destructor lives in the game-side derived class
		virtual ~INode() = default;
	};

}