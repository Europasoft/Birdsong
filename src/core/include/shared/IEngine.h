#pragma once
#include "core/include/shared/BoundaryUtils.h"

// SHARED INCLUDE

namespace EngineInterface
{
	class INode;

	// functions called by game code to be completed on the engine's side
	class IEngine
	{
	public:
		virtual void DLL_CALL getMousePosition(double& x, double& y) const = 0;
		virtual void DLL_CALL registerNode(INode* node) = 0;
		virtual void DLL_CALL unregisterNode(INode* node) = 0;

	protected:
		// prevent the game DLL from deleting the object
		// the actual destructor lives in the engine-side derived class
		~IEngine() = default; 
	};
}