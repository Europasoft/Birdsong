#include "core/draw/DrawBase.h"
#include "core/engine/Engine.h"
#include "core/gpu/Device.h"

namespace EngineCore
{
	using namespace WorldSystem;

	DrawBase::DrawBase(EngineDevice& device, const DrawContext& d)
		: device(device), d(d)
	{
	}

	DrawBase::~DrawBase() = default;

}