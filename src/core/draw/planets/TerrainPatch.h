#pragma once
#include "core/types/CommonTypes.h"
#include "core/engine/MeshData.h"

#include "core/types/vk.h"

#include <memory>
#include <vector>

namespace WorldSystem
{ 
	class EngineNodeData;
	class World;
}

namespace EngineCore
{
	class EngineDevice;
	class Material;
	class GBuffer;

	enum class ETerrainPatchFaceDirection : uint32_t { A, B, C, D, E, F };

	struct LargeVertex
	{
		Vec64 position{};
		Vec normal{};
		Vec color{};
	};

	class TerrainPatch
	{
	public:
		TerrainPatch(EngineDevice& device, uint32_t resolution, double radius);
		~TerrainPatch();

	public:
		// public data
		Vector2D<float> center; // local 2D face coordinates
		float size;  // extent in 2D space
		uint32_t lodLevel; // how deep inside the quadtree this patch is
		ETerrainPatchFaceDirection face; // +X, -X, +Y, etc.

		std::vector<std::unique_ptr<TerrainPatch>> children;

		const uint32_t resolution;
		const double radius;

	public:
		// public functions

		// split into 4 child patches (turning this leaf node into a container node)
		void split();

		// only leaf nodes have geometry and buffers
		bool isLeafNode() const;

		// create the patch vertices on CPU
		void generateGeometry();

		// copy the geometry into GPU memory
		void geometryToGPU();

		// to draw the final geometry
		void bindAndDraw(VkCommandBuffer commandBuffer) const;

	private:
		// private data

		EngineDevice& device;

		// CPU geometry buffers
		std::vector<LargeVertex> vertices{};
		std::vector<uint32_t> indices{};

		// GPU geometry buffers - only need one for each, since data is only written once
		std::unique_ptr<EngineCore::GBuffer> vertexBuffer = nullptr;
		std::unique_ptr<EngineCore::GBuffer> indexBuffer = nullptr;

	private:
		// private functions
		void createGPUBuffers();
		void destroyGPUBuffers();
		std::vector<Vertex> toSinglePrecision() const;

	};
}