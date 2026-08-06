// Copyright 2026 Simon Liimatainen. All rights reserved.
#pragma once
#include "core/types/CommonTypes.h"
#include "core/engine/MeshData.h"

#include "core/types/vk.h"

#include <memory>
#include <vector>
#include <atomic>
#include <future>

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
	class AsyncCommandBuffer;

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
		enum class EGenGeometryMode { UNIT_SPHERE_RELATIVE, PATCH_CENTER_RELATIVE };
	public:
		// public data
		const uint32_t resolution;
		const double radius;
		Vector2D<float> center; // local 2D face coordinates
		float size = 0;  // extent in 2D space
		uint32_t lodLevel = 0; // how deep inside the quadtree this patch is
		ETerrainPatchFaceDirection face = ETerrainPatchFaceDirection::A; // +X, -X, +Y, etc.

		std::vector<std::unique_ptr<TerrainPatch>> children;

		// if present, the updated patch to replace this one with
		// guarded by updaterMutex
		std::unique_ptr<TerrainPatch> next; 

	public:
		// public functions

		void splitReplace(AsyncCommandBuffer& commandBuffer, EGenGeometryMode genMode);
		void mergeReplace(AsyncCommandBuffer& commandBuffer, EGenGeometryMode genMode);
		// split into 4 child patches (turning this leaf node into a parent node)
		void split(std::vector<std::unique_ptr<JunkPileItem>>& junkPile, uint32_t frameIndex);

		// generate and push geometry to GPU
		void generate(AsyncCommandBuffer& commandBuffer, EGenGeometryMode genMode);

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
		EGenGeometryMode getCoordinateMode() const
		{
			return coordinateMode;
		}

		void scheduleFreeBuffers(std::vector<std::unique_ptr<JunkPileItem>>& junkPile, uint32_t frameIndex);
		void scheduleFreeBuffersRecursive(std::vector<std::unique_ptr<JunkPileItem>>& junkPile, uint32_t frameIndex);

		Vec64 cubeFaceToSphere(double faceX, double faceY) const;
		Vec64 getCenterDirection() const;

	private:
		// private data

		EngineDevice& device;

		std::atomic<EState> state;

		// CPU geometry buffers
		std::vector<LargeVertex> vertices{};
		std::vector<uint32_t> indices{};

		// GPU geometry buffers
		std::unique_ptr<TerrainPatchBuffers> buffers;

		EGenGeometryMode coordinateMode;

	private:
		// private functions

		// create the patch vertices on CPU
		void generateGeometry(EGenGeometryMode mode = EGenGeometryMode::UNIT_SPHERE_RELATIVE);
		// copy the geometry into GPU memory
		void geometryToGPU(AsyncCommandBuffer& commandBuffer);

		std::vector<Vertex> toSinglePrecision() const;

		void setState(EState s);

	};
}