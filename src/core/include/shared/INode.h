#pragma once
#include "core/include/shared/BoundaryUtils.h"

namespace EngineInterface
{
	// functions called by the engine executable
	class INode
	{
	public:
		virtual void DLL_CALL release() = 0;
		virtual void DLL_CALL onSpawnCall() = 0;
		virtual void DLL_CALL tickCall(float dt) = 0;
		virtual void DLL_CALL getTransform(uint8_t* buffer) const = 0;
		virtual void DLL_CALL setTransform(const uint8_t* buffer) = 0;
	protected:
		virtual ~INode() = default; // prevent the engine executable from deleting nodes created in DLL memory
	};

}