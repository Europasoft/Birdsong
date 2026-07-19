#pragma once
// include game interface from engine public headers (src/core/include/)
#include "game/Game.h"

#include <iostream>

class DemoGame : public EngineInterface::Game
{
public:
	virtual void onLoad() override;
	virtual void tick(float dt) override;
	virtual void onUnload() override;
};