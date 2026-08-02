// Copyright 2026 Simon Liimatainen. All rights reserved.
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
	class SingleTimeCommands;

	enum class ETerrainPatchFaceDirection : uint32_t { A, B, C, D, E, F };

	struct LargeVertex
	{
		Vec64 position{};
		Vec normal{};
		Vec color{};
	};

	struct TerrainPatchBuffers
	{
		// GPU geometry buffers - only need one for each, since data is only written once
		std::unique_ptr<EngineCore::GBuffer> vertexBuffer = nullptr;
		std::unique_ptr<EngineCore::GBuffer> indexBuffer = nullptr;
		std::unique_ptr<EngineCore::GBuffer> stagingBuffer1 = nullptr;
		std::unique_ptr<EngineCore::GBuffer> stagingBuffer2 = nullptr;
	};

	struct JunkPileItem
	{
		std::unique_ptr<TerrainPatchBuffers> buffers;
		uint32_t freeOnFrameIndex;
	};

	class TerrainPatch
	{
	public:
		TerrainPatch(EngineDevice& device, uint32_t resolution, double radius);
		~TerrainPatch();

		// only leaf nodes have valid geometry, but data may be pending or waiting to be freed
		enum class EState : uint32_t { PARENT, LOADING, LEAF };

	public:
		// public data
		Vector2D<float> center; // local 2D face coordinates
		float size = 0;  // extent in 2D space
		uint32_t lodLevel = 0; // how deep inside the quadtree this patch is
		ETerrainPatchFaceDirection face = ETerrainPatchFaceDirection::A; // +X, -X, +Y, etc.

		std::vector<std::unique_ptr<TerrainPatch>> children;

		const uint32_t resolution;
		const double radius;

	public:
		// public functions

		// split into 4 child patches (turning this leaf node into a parent node)
		bool split(std::vector<std::unique_ptr<JunkPileItem>>& junkPile, uint32_t frameIndex);

		// generate and push geometry to GPU
		void generate();

		bool updateReadiness();

		// bind and draw the final geometry if it is ready
		void draw(VkCommandBuffer commandBuffer);
	
		EState getState() const
		{ 
			return state; 
		}
		bool stateIs(EState s) const
		{
			return state == s;
		}
		bool isParent() const
		{
			return (state == EState::PARENT);
		}

		void scheduleFreeBuffers(std::vector<std::unique_ptr<JunkPileItem>>& junkPile, uint32_t frameIndex);
		void scheduleFreeBuffersRecursive(std::vector<std::unique_ptr<JunkPileItem>>& junkPile, uint32_t frameIndex);

	private:
		// private data

		EngineDevice& device;

		EState state;

		// CPU geometry buffers
		std::vector<LargeVertex> vertices{};
		std::vector<uint32_t> indices{};

		// GPU geometry buffers
		std::unique_ptr<TerrainPatchBuffers> buffers;


		std::unique_ptr<SingleTimeCommands> singleTimeCommands;

	private:
		// private functions

		// create the patch vertices on CPU
		void generateGeometry();
		// copy the geometry into GPU memory
		void geometryToGPU();

		void createGPUBuffers();
		std::vector<Vertex> toSinglePrecision() const;

		void setState(EState s);

	};
}