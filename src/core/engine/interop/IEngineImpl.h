#pragma once
#include "core/include/shared/IEngine.h"
#include "core/types/NodeContainer.h"
#include "core/include/shared/Transform.h"

namespace EngineInterface
{
	class INode;
}

struct Transform;

namespace EngineCore
{
	using IEngine = ::EngineInterface::IEngine;
	using INode = ::EngineInterface::INode;
	class EngineApplication;

	// engine-side implementation of IEngine
	class IEngineImpl : public IEngine
	{
	protected:
		// these functions can be invoked from the game DLL across the ABI boundary
		void DLL_CALL registerNode(INode* node) final override;
		void DLL_CALL unregisterNode(INode* node) final override;
		void DLL_CALL getMousePosition(double& x, double& y) const final override;

	private:
		static Transform getNodeTransform(INode* node);

	public:
		~IEngineImpl();
		EngineApplication* engine = nullptr;

	};
}