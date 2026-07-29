#pragma once
#include "core/engine/MeshData.h"
#include "core/types/CommonTypes.h"

#include <vector>
#include <cmath>
#include <array>

namespace EngineCore::Planets
{
	MeshBuilder generateSubFace(uint32_t faceIndex, uint32_t resolution, double radius, Vec264 offset, double scale, bool isRootFace=false);
	Vec64 projectToSphere(uint32_t face_index, Vec264 localCenter2D, double radius);
}