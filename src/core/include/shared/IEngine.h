#pragma once
#include "core/include/shared/BoundaryUtils.h"

namespace EngineInterface
{
	// functions called by game code to be completed on the engine's side
	class IEngine
	{
	public:
		virtual void DLL_CALL getMousePosition(double& x, double& y) const = 0;

	protected:
		// prevent the game DLL from deleting the object
		// the actual destructor lives in the engine-side derived class
		~IEngine() = default; 
	};
}