// Copyright 2026 Simon Liimatainen. All rights reserved.
#include "core/draw/planets/TerrainPatch.h"
#include "core/draw/planets/PlanetDrawer.h"
#include "core/gpu/Buffer.h"
#include "core/gpu/Device.h"
#include "core/gpu/Device.h"

#include <stdexcept>
#include <array>
#include <limits>
#include <utility>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

// glm
//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/gtc/constants.hpp>

#include "core/engine/EngineClock.h"

namespace EngineCore
{
	TerrainPatch::TerrainPatch(EngineDevice& device, uint32_t resolution, double radius)
		: device(device), resolution(resolution), radius(radius), state(EState::PARENT)
	{
		assert(resolution != 0 && radius != 0);
	}

	TerrainPatch::~TerrainPatch()
	{
	}

	void TerrainPatch::splitReplace(AsyncCommandBuffer& commandBuffer)
	{
		assert(stateIs(EState::LEAF) && children.size() == 0 && "cannot split terrain patch - not a leaf\n");

		// make a new patch to replace this one with
		assert(not next);
		next = std::make_unique<TerrainPatch>(device, resolution, radius);
		next->setState(EState::PARENT);
		next->center = center;
		next->size = size;
		next->lodLevel = lodLevel;
		next->face = face;


		const float child_size = size * 0.5f;
		const Vector2D<float> o = center;

		// 4 local sub-quadrants
		const Vector2D<float> offsets[4] =
		{
			{ o.x,               o.y               }, // bottom-left
			{ o.x + child_size,  o.y               }, // bottom-right
			{ o.x,               o.y + child_size  }, // top-left
			{ o.x + child_size,  o.y + child_size  }  // top-right
		};

		next->children.resize(4);
		for (uint32_t i = 0; i < 4; ++i)
		{
			next->children[i] = std::make_unique<TerrainPatch>(device, resolution, radius);
			TerrainPatch& child = *next->children[i];

			child.center = offsets[i];
			child.size = child_size;
			child.lodLevel = lodLevel + 1;
			child.face = face;

			// build child geometry (this is hard on performance)
			child.generate(commandBuffer);
		}
	}

	void TerrainPatch::mergeReplace(AsyncCommandBuffer& commandBuffer)
	{
		// make a new patch to replace this one with
		next = std::make_unique<TerrainPatch>(device, resolution, radius);
		next->center = center;
		next->size = size;
		next->lodLevel = lodLevel;
		next->face = face;
		
		next->generate(commandBuffer);
	}

	void TerrainPatch::split(std::vector<std::unique_ptr<JunkPileItem>>& junkPile, uint32_t frameIndex)
	{
		EngineClock clock{};
		assert(stateIs(EState::LEAF) && "cannot split terrain patch - not a leaf\n");
		assert(children.size() == 0);
		// free parent mesh data so it stops rendering and acts as a container node
		vertices.clear();
		indices.clear();
		scheduleFreeBuffers(junkPile, frameIndex);
		setState(EState::PARENT);

		float child_size = size * 0.5f;
		Vector2D<float> o = center;

		// 4 local sub-quadrants
		Vector2D<float> offsets[4] = {
			{ o.x,               o.y               }, // bottom-left
			{ o.x + child_size,  o.y               }, // bottom-right
			{ o.x,               o.y + child_size  }, // top-left
			{ o.x + child_size,  o.y + child_size  }  // top-right
		};

		children.resize(4);
		for (int i = 0; i < 4; ++i)
		{
			children[i] = std::make_unique<TerrainPatch>(device, resolution, radius);
			TerrainPatch& child = *children[i];

			child.center = offsets[i];
			child.size = child_size;
			child.lodLevel = lodLevel + 1;
			child.face = face;

			// build child geometry (this is hard on performance)
			//child.generate(VK_NULL_HANDLE);
		}
		//std::cout << "split took " << clock.getElapsed() << " seconds\n";
	}

	void TerrainPatch::generate(AsyncCommandBuffer& commandBuffer)
	{
		assert(not buffers);
		generateGeometry();
		geometryToGPU(commandBuffer);
	}

	void TerrainPatch::scheduleFreeBuffers(std::vector<std::unique_ptr<JunkPileItem>>& junkPile, uint32_t frameIndex)
	{
		junkPile.push_back(std::make_unique<JunkPileItem>(std::move(buffers), frameIndex));
		assert(children.size() == 0);
	}

	void TerrainPatch::scheduleFreeBuffersRecursive(std::vector<std::unique_ptr<JunkPileItem>>& junkPile, uint32_t frameIndex)
	{
		if (buffers) junkPile.push_back(std::make_unique<JunkPileItem>(std::move(buffers), frameIndex));
		for (auto& child : children)
		{
			child->scheduleFreeBuffersRecursive(junkPile, frameIndex);
		}
	}

	// GEOMETRY FUNCTIONS - ONLY RELEVANT FOR PATCHES THAT ARE LEAF NODES

	void TerrainPatch::draw(VkCommandBuffer commandBuffer)
	{
		assert(stateIs(EState::LEAF) && "cannot draw patch that has no geometry");
		// bind to command buffer
		const auto& v = buffers->vertexBuffer;
		const auto& i = buffers->indexBuffer;
		VkBuffer buffers[] = { v->getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, i->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		// send draw command
		vkCmdDrawIndexed(commandBuffer, indices.size(), 1, 0, 0, 0);
	}

	std::vector<Vertex> TerrainPatch::toSinglePrecision() const
	{
		std::vector<Vertex> verts;
		verts.reserve(vertices.size());

		for (const auto& v : vertices)
		{
			Vertex vert{};
			vert.position.x = static_cast<float>(v.position.x);
			vert.position.y = static_cast<float>(v.position.y);
			vert.position.z = static_cast<float>(v.position.z);
			vert.normal.x = v.normal.x;
			vert.normal.y = v.normal.y;
			vert.normal.z = v.normal.z;
			vert.color.x = v.color.x;
			vert.color.y = v.color.y;
			vert.color.z = v.color.z;
			vert.uv = { 0, 0 };
			verts.push_back(vert);
		}
		return verts;
	}

	void TerrainPatch::setState(EState s)
	{
		assert((s != EState::LEAF || children.size() == 0) && "patch with children cannot become leaf");
		state = s;
	}

	void TerrainPatch::geometryToGPU(AsyncCommandBuffer& commandBuffer)
	{
		EngineClock clock{};
		assert(stateIs(EState::LEAF));

		if (not buffers)
		{
			// create GPU buffers
			assert(vertices.size() && indices.size());
			buffers = std::make_unique<TerrainPatchBuffers>();
			buffers->vertexBuffer = std::make_unique<GBuffer>(device, sizeof(vertices[0]), vertices.size(),
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			buffers->indexBuffer = std::make_unique<GBuffer>(device, sizeof(indices[0]), indices.size(),
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}

		// let the GPU do the buffer copying at its own pace, check later if the commands are done so we can draw the vertices

		// convert double-precision vertex coordinates to single precision for GPU
		const std::vector<Vertex> standardVertices = TerrainPatch::toSinglePrecision();

		assert(standardVertices.size() >= 3 && "vertexCount cannot be below 3");
		VkDeviceSize bufferSize = sizeof(standardVertices[0]) * standardVertices.size();
		// temporary transfer buffer
		buffers->stagingBuffer1.reset();
		buffers->stagingBuffer1 = std::make_unique<GBuffer>
		(
			device, sizeof(standardVertices[0]), static_cast<uint32_t>(standardVertices.size()),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		buffers->stagingBuffer1->map();
		buffers->stagingBuffer1->writeToBuffer((void*)standardVertices.data()); // write vertices
		commandBuffer.copyBuffer(*buffers->stagingBuffer1, *buffers->vertexBuffer, bufferSize);

		// same as for vertex buffer
		bufferSize = sizeof(indices[0]) * indices.size();
		buffers->stagingBuffer2 = std::make_unique<GBuffer>
		(
			device, sizeof(indices[0]), static_cast<uint32_t>(indices.size()),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		buffers->stagingBuffer2->map();
		buffers->stagingBuffer2->writeToBuffer((void*)indices.data());
		commandBuffer.copyBuffer(*buffers->stagingBuffer2, *buffers->indexBuffer, bufferSize);

		const float ms = clock.getElapsed() * 1000;
		//if (ms > 0.15) std::cout << "============= geometryToGPU() done in " << ms << " ms =============\n";
	}

	std::array<float, 3> getPatchColor(uint32_t i)
	{
		static const std::array<std::array<float, 3>, 7> colors = { {{0.05,0.05,0.5}, {0.1,0.9,0.8}, {0.1,0.1,0.4}, {0.0,0.3,0.2}, {0.7,0.2,0.0}, {0.3,0.1,0.3}, {0.95, 0.75, 0.20}} };
		return colors[i % colors.size()];
	}

	std::array<float, 3> getPatchColor_visualizeLODLevel(uint32_t lod)
	{
		lod += 1;
		return std::array<float, 3>{ lod / 5.f, lod / 12.f, lod / 25.f };
	}

	void TerrainPatch::generateGeometry()
	{
		EngineClock clock{};
		assert(stateIs(EState::PARENT) && children.size() == 0);
		setState(EState::LEAF);
		const uint32_t faceIndex = static_cast<uint32_t>(face);
		static uint32_t debugColorIdx = 0;
		debugColorIdx++;

		// an nxn grid of quads requires (n+1) x (n+1) vertices.
		vertices.reserve((resolution + 1) * (resolution + 1));
		// each quad consists of 2 triangles = 6 indices. nxn quads * 6 = total indices
		indices.reserve(resolution * resolution * 6);

		// CUBE FACE BASIS VECTOR DEFINITIONS
		// orthonormal 3d axes defining the 2d plane coordinate system for each of the 6 cube faces
		// face layout: +x, -x, +y, -y, +z, -z
		static const std::array<Vec64, 6> rights = { {{0,0,1}, {0,0,-1}, {1,0,0}, {1,0,0}, {1,0,0}, {-1,0,0}} };
		static const std::array<Vec64, 6> ups = { {{0,1,0}, {0,1,0}, {0,0,1}, {0,0,-1}, {0,1,0}, {0,1,0}} };
		static const std::array<Vec64, 6> forwards = { {{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}} };

		// select the basis vectors corresponding to the specified faceIndex
		const auto& r = rights[faceIndex]; // local 2D x-axis on cube face
		const auto& u = ups[faceIndex]; // local 2D y-axis on cube face
		const auto& f = forwards[faceIndex]; // cube face normal vector (distance = 1 from origin)

		constexpr double PI_OVER_4 = 0.78539816339f; // constant PI / 4 used for equiangular warping: tan(PI/4) = 1, tan(-PI/4) = -1

		// VERTEX GENERATION LOOP
		for (uint32_t y = 0; y <= resolution; ++y)
		{
			// convert y loop index to local normalized coordinate [0.0, 1.0]
			double local_y = static_cast<double>(y) / resolution;

			// apply scale and quadtree offset to compute coordinate in range [-1.0, 1.0]
			double my = center.y + local_y * size;

			// apply equiangular (tangent) distortion mapping to y axis
			// this converts uniform linear spacing into uniform angular spacing on the sphere,
			// preventing area compression/distortion at cube corners
			double tan_y = std::tan(my * PI_OVER_4);

			for (uint32_t x = 0; x <= resolution; ++x)
			{
				// convert x loop index to local normalized coordinate [0.0, 1.0]
				double local_x = static_cast<double>(x) / resolution;

				// !! TODO: ASAP: looks like x and y  are flipped here:

				// (map into local face space [-1, 1])
				// map to cube face range [-1.0, 1.0] using offset and scale
				double mx = center.x + local_x * size;

				// apply equiangular distortion mapping to x-axis
				double tan_x = std::tan(mx * PI_OVER_4);

				// construct 3D point (cx, cy, cz) on the surface of the unit cube:
				// center_point + (right_vector * tan_x) + (up_vector * tan_y)
				double cx = f.x + r.x * tan_x + u.x * tan_y;
				double cy = f.y + r.y * tan_x + u.y * tan_y;
				double cz = f.z + r.z * tan_x + u.z * tan_y;


				// projects point onto unit sphere by using inverse magnitude to normalize the vector
				double inv_len = 1.0f / std::sqrt(cx * cx + cy * cy + cz * cz);
				// unit vector pointing outwards from sphere center (also doubles as surface normal)
				Vec64 n = { cx * inv_len, cy * inv_len, cz * inv_len };
				double nx = cx * inv_len;
				double ny = cy * inv_len;
				double nz = cz * inv_len;

				LargeVertex v;
				v.position = n; // instead of scaling by radius here, just make it a unit sphere and let the GPU scale it up
				v.normal = { static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z) };

				auto col = getPatchColor_visualizeLODLevel(lodLevel);
				v.color = { col[0], col[1], col[2] };

				vertices.push_back(v);
			}
		}

		// INDEX GENERATION LOOP (TRIANGULATION)
		// distance in vertex array between adjacent vertical rows — indices are relative to this patch's local mesh
		int stride = resolution + 1;
		for (int y = 0; y < resolution; ++y)
		{
			for (int x = 0; x < resolution; ++x)
			{
				uint32_t i0 = x + y * stride;
				uint32_t i1 = (x + 1) + y * stride;
				uint32_t i2 = x + (y + 1) * stride;
				uint32_t i3 = (x + 1) + (y + 1) * stride;

				indices.push_back(i0);
				indices.push_back(i2);
				indices.push_back(i1);

				indices.push_back(i1);
				indices.push_back(i2);
				indices.push_back(i3);
			}
		}
		const float ms = clock.getElapsed() * 1000;
		//if (ms > 0.15) std::cout << "============= generateGeometry() done in " << ms << " ms =============\n";
	}
	
}