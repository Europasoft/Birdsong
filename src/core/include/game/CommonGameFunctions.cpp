#include "game/CommonGameFunctions.h"
#include "shared/BoundaryUtils.h"
#include "shared/Transform.h"

#include <vector>

// GAME-ONLY SOURCE

void CommonGameFunctions::physicsExplode(const Transform& position, float falloff, float radius, float impulsePerArea)
{
	const auto buffer = BoundaryUtils::transformToBuffer(position);
	engine->physicsExplode(buffer.data(), falloff, radius, impulsePerArea);
}
