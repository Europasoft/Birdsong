#pragma once
#include "core/include/shared/BoundaryUtils.h"

namespace EngineInterface
{
	// lifecycle functions called by the engine executable
	class IGame
	{
	public:
		virtual void DLL_CALL onLoadCall() = 0;
		virtual void DLL_CALL onTickCall(float dt) = 0;
		virtual void DLL_CALL onUnloadCall() = 0;
		virtual void DLL_CALL release() = 0; // guarantees deletion happens inside the DLL

	protected:
		virtual ~IGame() = default; // prevent the engine executable from deleting the game object
	};

}