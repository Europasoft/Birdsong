#pragma once
#include "core/engine/MeshData.h"


#include <vector>
#include <cmath>
#include <array>

namespace EngineCore::Planets
{
	// Generates a single face of a quad-sphere at root resolution (LOD 0)
	MeshBuilder generateCubeFace(int face_index, int resolution, float radius);
	MeshBuilder generateSubFace(int face_index, int resolution, float radius,
					std::array<float, 2> offset, float scale, bool isRootFace=false);
}