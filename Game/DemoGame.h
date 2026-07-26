#pragma once
// include game interface from engine public headers (src/core/include/)
#include "game/Game.h"

#include <vector>
#include <memory>

namespace EngineInterface
{
	class MeshNode;
}

class DemoGame : public EngineInterface::Game
{
public:
	virtual void onLoad() override;
	virtual void tick(double dt) override;
	virtual void onUnload() override;

private:
	std::vector<std::unique_ptr<EngineInterface::MeshNode>> demoMeshes;
};