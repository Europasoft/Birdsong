#pragma once

// include game interface from engine public headers (src/core/include)
#include "IGame.h"

#include <iostream>

class Game : public EngineInterface::IGame
{
public:
	void onLoad() override;

	void onUpdate(float deltaTime) override;

	void onUnload() override;

	void release() override;
};