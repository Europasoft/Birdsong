#pragma once
#include "core/include/game/Node.h"

// GAME-ONLY INCLUDE

#include <string_view>

// TODO: game-side node classes need not be in an interface
namespace EngineInterface
{
	class MeshNode : public Node
	{
	public:
		using Node::Node;

	public:
		void setMesh(std::string_view filepath = "Meshes/teapot.obj");
	};
}