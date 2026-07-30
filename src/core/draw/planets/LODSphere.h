#pragma once
#include "core/engine/MeshData.h"
#include "core/types/CommonTypes.h"

#include <vector>
#include <cmath>
#include <array>

namespace EngineCore::Planets
{
	struct LargeVertex
	{
		Vec64 position{};
		Vec normal{};
		Vec color{};
	};

	struct LargeGeometry
	{
		std::vector<LargeVertex> vertices{};
		std::vector<uint32_t> indices{};
		EngineCore::MeshBuilder toSinglePrecision() const;
	};

	LargeGeometry generateSubFace(uint32_t faceIndex, uint32_t resolution, double radius, Vec264 offset, double scale, uint32_t lodLevel, bool isRootFace=false);
	Vec64 projectToSphere(uint32_t face_index, Vec264 localCenter2D, double radius);
}