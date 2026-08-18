// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once
#include "core/draw/DrawBase.h"

namespace EngineCore
{
	class InterfaceDrawer : public DrawBase
	{
	public:
		InterfaceDrawer(EngineDevice& device, const DrawContext& d);
		~InterfaceDrawer();

		virtual void render(const FrameContext& f) override;
	};

}