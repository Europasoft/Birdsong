#pragma once
#include "core/include/shared/IEngine.h"

namespace EngineCore
{
	using IEngine = ::EngineInterface::IEngine;
	class EngineApplication;

	// engine-side implementation of IEngine
	class IEngineImpl : public IEngine
	{
	protected:
		// these functions can be invoked across the ABI boundary from the game DLL
		void DLL_CALL getMousePosition(double& x, double& y) const final override;

	public:
		~IEngineImpl();

		EngineApplication* engine = nullptr;

	};
}