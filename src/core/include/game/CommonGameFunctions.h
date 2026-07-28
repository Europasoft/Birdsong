#pragma once
#include "shared/IEngine.h"

// GAME-ONLY INCLUDE

namespace EngineInterface
{
	class Game;
}

struct Transform;

class CommonGameFunctions
{
protected:
	EngineInterface::IEngine* engine = nullptr;
	friend class EngineInterface::Game;

public:
	void physicsExplode(const Transform& position, float falloff, float radius, float impulsePerArea);

};