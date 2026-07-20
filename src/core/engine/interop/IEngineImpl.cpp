#include "core/engine/interop/IEngineImpl.h"
#include "core/engine/Engine.h"

namespace EngineCore
{
	IEngineImpl::~IEngineImpl()
	{}

	void IEngineImpl::getMousePosition(double& x, double& y) const
	{
		const auto mp = engine->window.input.getMousePosition();
		x = mp.x;
		y = mp.y;
	}
}